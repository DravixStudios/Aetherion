#pragma once
#include "Core/Renderer/GPUTexture.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>


class VulkanTexture : public GPUTexture {
public:
	VulkanTexture(VkDevice& dev, VkPhysicalDevice& physicalDev, VkImage& buffer, VkDeviceMemory& memory, uint32_t nSize);

	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkImage GetBuffer();
	VkDeviceMemory GetMemory();

	uint32_t GetSize() override;
private:
	VkDevice m_dev;
	VkPhysicalDevice m_physicalDev;
	VkImage m_buffer;
	VkDeviceMemory m_memory;

	uint32_t m_nSize;
};