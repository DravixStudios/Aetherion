#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "Core/Renderer/GPUBuffer.h"

class VulkanBuffer : public GPUBuffer {
public:
	VulkanBuffer(VkDevice& dev, VkPhysicalDevice& physicalDev, VkBuffer& buffer, VkDeviceMemory& memory, uint32_t nSize, EBufferType bufferType);
	~VulkanBuffer();

	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkBuffer GetBuffer();
	VkDeviceMemory GetMemory();

	uint32_t GetSize() override;

private:
	VkDevice m_dev;
	VkPhysicalDevice m_physicalDev;
	VkBuffer m_buffer;
	VkDeviceMemory m_memory;

	uint32_t m_nSize;
};