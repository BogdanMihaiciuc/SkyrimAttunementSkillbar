#include "Renderer.h"

#include <d3d11.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <dxgi.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../include/external/stb_image.h"

#include "imgui_internal.h"

#include "../SkillHUD/SkillHUDPresentationController.h"
#include "../SkillHUD/SkillAssignmentHUD.h"
#include "../Input/SpellCastController.h"

namespace stl
{
	template <class T>
	void write_thunk_call()
	{
		auto& trampoline = SKSE::GetTrampoline();
		REL::Relocation<std::uintptr_t> hook{ T::id, T::offset };
		T::func = trampoline.write_call<5>(hook.address(), T::thunk);
	}
}  // namespace stl

#pragma push_macro("ERROR")
#undef ERROR

#define INFO(...) logger::info(__VA_ARGS__)
#define ERROR(...) logger::error(__VA_ARGS__)

namespace AttunementSkillbar {

	LRESULT Renderer::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto& io = ImGui::GetIO();
		if (uMsg == WM_KILLFOCUS) {
			io.ClearInputCharacters();
			io.ClearInputKeys();
		}

		return func(hWnd, uMsg, wParam, lParam);
	}

	void Renderer::D3DInitHook::thunk() {
		func();

		INFO("D3DInit Hooked!");
		auto render_manager = RE::BSRenderManager::GetSingleton();
		if (!render_manager) {
			ERROR("Cannot find render manager. Initialization failed!");
			return;
		}

		auto render_data = render_manager->GetRuntimeData();

		INFO("Getting swapchain...");
		auto swapchain = render_data.swapChain;
		if (!swapchain) {
			ERROR("Cannot find swapchain. Initialization failed!");
			return;
		}

		INFO("Getting swapchain desc...");
		DXGI_SWAP_CHAIN_DESC sd{};
		if (swapchain->GetDesc(std::addressof(sd)) < 0) {
			ERROR("IDXGISwapChain::GetDesc failed.");
			return;
		}

		device = render_data.forwarder;
		context = render_data.context;

		INFO("Initializing ImGui...");
		ImGui::CreateContext();
		if (!ImGui_ImplWin32_Init(sd.OutputWindow)) {
			ERROR("ImGui initialization failed (Win32)");
			return;
		}
		if (!ImGui_ImplDX11_Init(device, context)) {
			ERROR("ImGui initialization failed (DX11)");
			return;
		}

		RECT rect = { 0, 0, 0, 0 };
		GetClientRect(sd.OutputWindow, &rect);
		ImGui::GetIO().DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

		INFO("ImGui initialized!");

		initialized.store(true);

		WndProcHook::func = reinterpret_cast<WNDPROC>(
			SetWindowLongPtrA(
				sd.OutputWindow,
				GWLP_WNDPROC,
				reinterpret_cast<LONG_PTR>(WndProcHook::thunk)
			)
		);

		if (!WndProcHook::func) {
			ERROR("SetWindowLongPtrA failed!");
		}

		// If ImGui icons is installed, load the jost font to be used where text is drawn
		auto fontPath = "Data\\Interface\\ImGuiIcons\\Fonts\\Jost-Medium.ttf";
		bool hasFont = false;
		if (std::filesystem::exists(fontPath) && std::filesystem::is_regular_file(fontPath)) {
			hasFont = true;
		}

		if (hasFont) {
			auto IO = ImGui::GetIO();
			auto scale = std::min(IO.DisplaySize.x / 1920.0f, IO.DisplaySize.y / 1080.0f);
			ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath, 64.0f * scale);
		}
	}

	#ifdef RenderDXGIHook

		void Renderer::DXGIPresentHook::thunk(std::uint32_t a_p1) {
			func(a_p1);

			if (!D3DInitHook::initialized.load())
				return;

			// prologue
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			// do stuff
			Renderer::DrawSkillbar();

			// epilogue
			ImGui::EndFrame();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

	#else

		void Renderer::MenuPresentHook::Hook_PostDisplay(RE::IMenu* Menu) {
			if (D3DInitHook::initialized.load()) {
				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				Renderer::DrawSkillbar();

				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());  // dear imgui defaults to RT slot 0
			}

			func(Menu);
		}

	#endif

	// Simple helper function to load an image into a DX11 texture with common settings
	bool Renderer::LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, std::int32_t& out_width, std::int32_t& out_height) {
		auto render_manager = RE::BSRenderManager::GetSingleton();
		if (!render_manager) {
			ERROR("Cannot find render manager. Initialization failed!");
			return false;
		}

		auto render_data = render_manager->GetRuntimeData();

		// Load from disk into a raw RGBA buffer
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		// Create texture
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = image_width;
		desc.Height = image_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = NULL;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = image_data;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;
		device->CreateTexture2D(&desc, &subResource, &pTexture);

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		render_data.forwarder->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		pTexture->Release();

		out_width = image_width;
		out_height = image_height;
		stbi_image_free(image_data);

		return true;
	}

	void Renderer::DrawSkillbar() {
		#ifndef SpellCastMainLoopHook
			// If spell cast controller doesn't use its own hook, use this entry point to
			// also update the spell cast state
			SpellCastController::SharedController()->ProcessFrame();
		#endif

		static constexpr ImGuiWindowFlags windowFlag = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;

		// If the assignment HUD is open, draw it regardless of the UI status
		auto assignmentHUD = SkillAssignmentHUD::SharedHUD();
		if (assignmentHUD->IsOpen()) {
			float screenSizeX = ImGui::GetIO().DisplaySize.x;
			float screenSizeY = ImGui::GetIO().DisplaySize.y;

			ImGui::SetNextWindowSize(ImVec2(screenSizeX, screenSizeY));
			ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));

			ImGui::Begin("BMAttunementSkillbar", nullptr, windowFlag);

			assignmentHUD->Render();
			
			ImGui::End();
			return;
		}

		auto UI = RE::UI::GetSingleton();
		if (!UI || UI->GameIsPaused() || !UI->IsCursorHiddenWhenTopmost() || !UI->IsShowingMenus() || !UI->GetMenu<RE::HUDMenu>()) {
			return;
		}

		float screenSizeX = ImGui::GetIO().DisplaySize.x;
		float screenSizeY = ImGui::GetIO().DisplaySize.y;

		ImGui::SetNextWindowSize(ImVec2(screenSizeX, screenSizeY));
		ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));

		ImGui::Begin("BMAttunementSkillbar", nullptr, windowFlag);

		SkillHUDPresentationController::SharedController()->Render();
		
		ImGui::End();
	}

	void Renderer::MessageCallback(SKSE::MessagingInterface::Message* msg) {
		if (msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
		}
	}

	bool Renderer::Install() {
		auto g_message = SKSE::GetMessagingInterface();
		if (!g_message) {
			ERROR("Messaging Interface Not Found!");
			return false;
		}

		g_message->RegisterListener(MessageCallback);

		stl::write_thunk_call<D3DInitHook>();
		#ifdef RenderDXGIHook
			stl::write_thunk_call<DXGIPresentHook>();
		#else
			MenuPresentHook::Install();
		#endif

		return true;
	}

	float Renderer::GetResolutionScaleWidth() {
		return ImGui::GetIO().DisplaySize.x / 1920.f;
	}

	float Renderer::GetResolutionScaleHeight() {
		return ImGui::GetIO().DisplaySize.y / 1080.f;
	}

}

#pragma pop_macro("ERROR")