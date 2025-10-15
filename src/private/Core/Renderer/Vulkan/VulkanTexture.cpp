#include "Core/Renderer/Vulkan/VulkanTexture.h"

VulkanTexture::VulkanTexture(
	VkDevice& dev, 
	VkPhysicalDevice& physicalDev,
	VkImage& buffer,
	VkDeviceMemory& memory,
	uint32_t nSize,
	VkImageView& imageView,
	VkSampler& sampler
) : GPUTexture::GPUTexture() {
	this->m_dev = dev;
	this->m_physicalDev = physicalDev;
	this->m_buffer = buffer;
	this->m_memory = memory;
	this->m_nSize = nSize;
	this->m_imageView = imageView;
	this->m_sampler = sampler;
}

VkDevice VulkanTexture::GetDevice() {
	if (this->m_dev == nullptr) {
		spdlog::error("VulkanTexture::GetDevice: Device not defined");
	}

	return this->m_dev;
}

VkPhysicalDevice VulkanTexture::GetPhysicalDevice() {
	if (this->m_physicalDev == nullptr) {
		spdlog::error("VulkanTexture::GetPhysicalDevice: Physical not defined");
	}

	return this->m_physicalDev;
}

VkImage VulkanTexture::GetBuffer() {
	if (this->m_buffer == nullptr) {
		spdlog::error("VulkanTexture::GetBuffer: Buffer not defined");
	}

	return this->m_buffer;
}

VkDeviceMemory VulkanTexture::GetMemory() {
	if (this->m_memory == nullptr) {
		spdlog::error("VulkanTexture::GetMemory: DeviceMemory not defined");
	}

	return this->m_memory;
}

uint32_t VulkanTexture::GetSize() {
	if (this->m_nSize <= 0) {
		spdlog::error("VulkanTexture::GetSize: Buffer size is 0");
	}

	return this->m_nSize;
}