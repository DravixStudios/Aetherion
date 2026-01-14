#pragma once
#include "Core/Renderer/Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanRenderer : public Renderer {
public:
	static constexpr const char* CLASS_NAME = "VulkanRenderer";

	explicit VulkanRenderer();
	~VulkanRenderer() override;

	void Create() override;

private:
	bool m_bEnableValidationLayers;

	bool CheckValidationLayersSupport();
	Vector<const char*> GetRequiredExtensions();

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
		VkDebugUtilsMessageTypeFlagsEXT type, 
		const VkDebugUtilsMessengerCallbackDataEXT* pcData, 
		void* pvUserData
	);

	VkInstance m_instance;
};