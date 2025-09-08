#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Core/Renderer/Renderer.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <cstring>

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS 1
#else
#define ENABLE_VALIDATION_LAYERS 0
#endif // NDEBUG

class VulkanRenderer : public Renderer {
public:
	VulkanRenderer();
	
	void Init() override;
	void Update() override;
private:
	bool m_bEnableValidationLayers;

	VkInstance m_vkInstance;
	
	void CreateInstance();

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCbData,
		void* pvUserData
	);

	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayersSupport();
};
