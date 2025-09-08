#include <iostream>
#include <spdlog/spdlog.h>
#include "Core/Core.h"

Core* g_core = nullptr;

int main() {
	g_core = Core::GetInstance();

	if (g_core == nullptr) {
		spdlog::error("Failed to get the Core instance");
		return 1;
	}

	g_core->Init();
	g_core->Update();

	return 0;
}