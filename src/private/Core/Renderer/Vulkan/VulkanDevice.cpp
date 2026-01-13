#include "Core/Renderer/Vulkan/VulkanDevice.h"

VulkanDevice::VulkanDevice(
	VkPhysicalDevice physicalDevice, 
	VkInstance instance, 
	VkSurfaceKHR surface
) : m_physicalDevice(physicalDevice), m_instance(instance), m_surface(surface), m_device(VK_NULL_HANDLE) { }

VulkanDevice::~VulkanDevice() {
	if (this->m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(this->m_device, nullptr);
	}
}