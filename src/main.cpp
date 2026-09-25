#include "./Input/InputEventController.h"
#include "./Input/SpellCastController.h"
#include "./Renderer/Renderer.h"
#include "./SkillHUD/SkillHUD.h"
#include "./Configuration/ConfigurationController.h"
#include "./Configuration/PersistenceController.h"

static void MessageHandler(SKSE::MessagingInterface::Message* msg) {
	switch (msg->type) {
        case SKSE::MessagingInterface::kNewGame:
            [[fallthrough]];
        case SKSE::MessagingInterface::kPreLoadGame:
            AttunementSkillbar::InputEventController::SetGameLoading(true);
            break;
        case SKSE::MessagingInterface::kPostLoadGame:
            AttunementSkillbar::InputEventController::SetGameLoading(false);
            break;
	    case SKSE::MessagingInterface::kDataLoaded:
            // Load the skill textures and initialize the skill HUD with the default configuration
            AttunementSkillbar::SkillHUD::Initialize();
            AttunementSkillbar::SkillHUD::SharedHUD()->UseDefaultConfiguration();
            AttunementSkillbar::InputEventController::SharedController()->InitializeDodgeKey();

            // Begin listening for key presses and menu events
            RE::BSInputDeviceManager::GetSingleton()->AddEventSink(AttunementSkillbar::InputEventController::SharedController());
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(AttunementSkillbar::InputEventController::SharedController());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESEquipEvent>(AttunementSkillbar::SpellCastController::SharedController());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESContainerChangedEvent>(AttunementSkillbar::SkillHUD::SharedHUD());
            RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESSpellCastEvent>(AttunementSkillbar::SkillHUD::SharedHUD());
            break;
    }
}

void InitializeLog() {
#ifndef NDEBUG
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto path = logger::log_directory();
    if (!path) {
        SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
    }

    *path /= "AttunementSkillbar.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif
    const auto level = spdlog::level::trace;

    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
    log->set_level(level);
    log->flush_on(level);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::AllocTrampoline(1 << 10);

    InitializeLog();
    
    SKSE::Init(skse);

    AttunementSkillbar::PersistenceController::Install();

    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

    AttunementSkillbar::InputEventController::InstallHooks();
    AttunementSkillbar::SpellCastController::InstallHooks();
    AttunementSkillbar::Renderer::Install();
    
    // Prepare the MCM configuration
    SKSE::GetPapyrusInterface()->Register(AttunementSkillbar::ConfigurationController::InitializeScript);

    return true;
}