#include <iostream>
#include "Core/Core.h"
#include "Utils.h"
#include "Core/Logger.h"

#if defined(LOGGING_USE_SPDLOG)
	#include <spdlog/spdlog.h>
#endif

Core* g_core = nullptr;

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#ifdef _WIN32
#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#else
#pragma comment(linker, "/SUBSYSTEM:console")
#endif
#endif

int main() {
	g_core = Core::GetInstance();

#if defined(LOGGING_USE_SPDLOG)
	spdlog::set_level(spdlog::level::debug);
#ifndef NDEBUG
#endif // NDEBUG
#endif // LOGGING_USE_SPDLOG
	Logger::Info("Aetherion debug console");
	Logger::Info("Version: {0}.{1}.{2}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	Logger::Info("Dravix Studios. All rights reserved");

	if (g_core == nullptr) {
		spdlog::error("Failed to get the Core instance");
		return 1;
	}

	g_core->Init();
	g_core->Update();

	return 0;
}