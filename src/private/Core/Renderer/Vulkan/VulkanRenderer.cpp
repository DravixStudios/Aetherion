#include "Core/Renderer/Vulkan/VulkanRenderer.h"

Vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VulkanRenderer::VulkanRenderer() : m_instance(VK_NULL_HANDLE), m_bEnableValidationLayers(false) {}

VulkanRenderer::~VulkanRenderer() {
	if (this->m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(this->m_instance, nullptr);
		this->m_instance = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::Create() {
#ifndef NDEBUG
	this->m_bEnableValidationLayers = true;
#endif // NDEBUG

	
}

/**
* Check for validation layers support
* 
* @returns The result of the check
*/
bool 
VulkanRenderer::CheckValidationLayersSupport() {
	uint32_t nLayerCount;
	vkEnumerateInstanceLayerProperties(&nLayerCount, nullptr);

	Vector<VkLayerProperties> availableLayers(nLayerCount);
	vkEnumerateInstanceLayerProperties(&nLayerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool bLayerFound = false;

		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				bLayerFound = true;
				break;
			}
		}

		if (!bLayerFound) return false;
	}

	return true;
}

/**
* Gets a list of required extensions
* 
* @returns A vector of required extension names
*/
Vector<const char*>
VulkanRenderer::GetRequiredExtensions() {
	uint32_t nGlfwExtensions;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&nGlfwExtensions);

	Vector<const char*> extensions(glfwExtensions, glfwExtensions + nGlfwExtensions);

	if (this->m_bEnableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

/**
* Populates debug messenger create info
* 
* @param messengerInfo Reference to Debug messenger create info
*/
void
VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo) {
	messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	   |
								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerInfo.pfnUserCallback = VulkanRenderer::DebugCallback;
	messengerInfo.pUserData = this;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanRenderer::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pcData,
	void* pvUserData
) {
	VulkanRenderer* renderer = static_cast<VulkanRenderer*>(pvUserData);

	switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			Logger::Debug("Validation layers: {}", pcData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			Logger::Warn("Validation layers: {}", pcData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			Logger::Error("Validation layers: {}", pcData->pMessage);
			break;
	}

	return VK_FALSE;
}