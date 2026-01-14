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