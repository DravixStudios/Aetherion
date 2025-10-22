#pragma once
#include "Core/Renderer/GPURingBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

/* Forward declarations */
class VulkanRenderer;

class VulkanRingBuffer: public GPURingBuffer {
public:
	VulkanRingBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice);

	void Init(uint32_t nBufferSize, uint32_t nAlignment, uint32_t nFramesInFlight) override;
	void* Allocate(uint32_t nDataSize, uint64_t& outOffset) override;
	uint32_t Align(uint32_t nValue, uint32_t nAlignment) override;
	void Reset(uint32_t nImageIndex) override;

private:
	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;

	uint32_t m_nPerFrameSize;
	uint32_t m_nOffset;

	VulkanRenderer* m_vkRenderer;

	VkBuffer m_buffer;
	VkDeviceMemory m_memory;

	void* pMap;
};