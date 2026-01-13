#include "Core/Renderer/Vulkan/VulkanDevice.h"

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool
	IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

VulkanDevice::VulkanDevice(
	VkPhysicalDevice physicalDevice, 
	VkInstance instance, 
	VkSurfaceKHR surface
) : m_physicalDevice(physicalDevice),
m_instance(instance), 
m_surface(surface), 
m_device(VK_NULL_HANDLE),
m_devProperties(0) { }

VulkanDevice::~VulkanDevice() {
	if (this->m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(this->m_device, nullptr);
	}
}

/**
* Creates a Vulkan logical device
* 
* @param createInfo Device create info
*/
void 
VulkanDevice::Create(const DeviceCreateInfo& createInfo) {
	this->CachePhysicalDeviceProperties();

	VkDeviceCreateInfo deviceInfo = { };
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	
}

/**
* Caches Vulkan physical device properties
*/
void
VulkanDevice::CachePhysicalDeviceProperties() {
	VkPhysicalDeviceProperties devProps = { };
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &devProps);

	this->m_devProperties = devProps;
}