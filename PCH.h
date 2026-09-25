#pragma once

// This file is required.
#undef ENABLE_SKYRIM_VR

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
#include "SKSE/SKSE.h"
#include <ShlObj_core.h>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif

#define SpellCastMainLoopHook
#define CasterBasedAftercastDelay

namespace logger = SKSE::log;

using namespace std::literals;
using namespace REL::literals;

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()
