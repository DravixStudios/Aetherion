#include "Core/Renderer/Vulkan/VulkanCommandPool.h"

VulkanCommandPool::VulkanCommandPool(VkDevice device) 
	: m_device(device), m_pool(VK_NULL_HANDLE) {}

VulkanCommandPool::~VulkanCommandPool() {
	if (this->m_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(this->m_device, this->m_pool, nullptr);
	}
}

/**
* Creates a Vulkan command pool
* 
* @param createInfo Command pool create info
*/
void 
VulkanCommandPool::Create(const CommandPoolCreateInfo& createInfo) {
	
}