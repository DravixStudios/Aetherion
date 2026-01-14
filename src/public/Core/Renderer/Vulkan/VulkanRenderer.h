#pragma once
#include "Core/Renderer/Renderer.h"
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities = { };
	Vector<VkSurfaceFormatKHR> formats;
	Vector<VkPresentModeKHR> presentModes;
};

class VulkanRenderer : public Renderer {
public:
	using Ptr = Ref<VulkanRenderer>;

	static constexpr const char* CLASS_NAME = "VulkanRenderer";

	explicit VulkanRenderer();
	~VulkanRenderer() override;

	void Create(GLFWwindow* pWindow) override;

	static Ptr
	CreateShared() {
		return CreateRef<VulkanRenderer>();
	}

private:
	GLFWwindow* m_pWindow;

	bool m_bEnableValidationLayers;
	VkInstance m_instance;
	
	VkSurfaceKHR m_surface;

	VkDebugUtilsMessengerEXT m_debugMessenger;

	VkPhysicalDevice m_physicalDevice;

	void PickPhysicalDevice();
	bool IsDeviceSuitable(const VkPhysicalDevice& physicalDevice);
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice);
	void CheckDescirptorIndexingSupport();

	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice);
	SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice);

	bool CheckValidationLayersSupport();
	Vector<const char*> GetRequiredExtensions();

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
		VkDebugUtilsMessageTypeFlagsEXT type, 
		const VkDebugUtilsMessengerCallbackDataEXT* pcData, 
		void* pvUserData
	);

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		VkAllocationCallbacks* pAllocator, 
		VkDebugUtilsMessengerEXT* pDebugMessenger
	);

};