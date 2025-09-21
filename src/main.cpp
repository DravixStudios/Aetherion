#include <iostream>
#include <spdlog/spdlog.h>
#include "Core/Core.h"

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

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#endif
	spdlog::info("Aetherion debug console");
	spdlog::info("Version: {0}.{1}.{2}", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	spdlog::info("Dravix Studios. All rights reserved");

	if (g_core == nullptr) {
		spdlog::error("Failed to get the Core instance");
		return 1;
	}

	g_core->Init();
	g_core->Update();

	return 0;
}