#include "Core/Renderer/Vulkan/VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(VkDevice& dev, VkPhysicalDevice& physicalDev, VkBuffer& buffer, VkDeviceMemory& memory, uint32_t nSize, EBufferType bufferType) : GPUBuffer() {
	this->m_dev = dev;
	this->m_physicalDev = physicalDev;
	this->m_buffer = buffer;
	this->m_memory = memory;
	this->m_nSize = nSize;
	this->m_bufferType = bufferType;
}

VkDevice VulkanBuffer::GetDevice() {
	if (this->m_dev == nullptr) {
		spdlog::error("VulkanBuffer::GetDevice: Device not defined");
	}

	return this->m_dev;
}

VkPhysicalDevice VulkanBuffer::GetPhysicalDevice() {
	if (this->m_physicalDev == nullptr) {
		spdlog::error("VulkanBuffer::GetPhysicalDevice: Physical not defined");
	}

	return this->m_physicalDev;
}

VkBuffer VulkanBuffer::GetBuffer() {
	if (this->m_buffer == nullptr) {
		spdlog::error("VulkanBuffer::GetBuffer: Buffer not defined");
	}

	return this->m_buffer;
}

VkDeviceMemory VulkanBuffer::GetMemory() {
	if (this->m_memory == nullptr) {
		spdlog::error("VulkanBuffer::GetMemory: DeviceMemory not defined");
	}

	return this->m_memory;
}

uint32_t VulkanBuffer::GetSize() {
	if (this->m_nSize <= 0) {
		spdlog::error("VulkanBuffer::GetSize: Buffer size is 0");
	}

	return this->m_nSize;
}