#include "Core/Renderer/Vulkan/VulkanSemaphore.h"

VulkanSemaphore::VulkanSemaphore(Ref<VulkanDevice> device) 
	: m_device(device), m_semaphore(VK_NULL_HANDLE) { }

VulkanSemaphore::~VulkanSemaphore() {
	VkDevice vkDevice = this->m_device.As<VulkanDevice>()->GetVkDevice();

	if (this->m_semaphore != VK_NULL_HANDLE) {
		vkDestroySemaphore(vkDevice, this->m_semaphore, nullptr);
	}
}

/**
* Creates Vulkan a semaphore
*/
void
VulkanSemaphore::Create() {
	VkDevice vkDevice = this->m_device.As<VulkanDevice>()->GetVkDevice();
	
	VkSemaphoreCreateInfo semaphoreInfo = { };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK(
		vkCreateSemaphore(
			vkDevice, 
			&semaphoreInfo, 
			nullptr,
			&this->m_semaphore
		), 
		"Failed to create semaphore");
}