#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "Core/Renderer/GPUBuffer.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"

class VulkanBuffer : public GPUBuffer {
public:
	using Ptr = Ref<VulkanBuffer>;

	explicit VulkanBuffer(VkDevice device);
	~VulkanBuffer() override;

	void Create(const void* pcData, uint32_t nSize, EBufferType bufferType = EBufferType::VERTEX_BUFFER) override;

	VkBuffer GetVkBuffer() const { return this->m_buffer; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanBuffer>(device);
	}

private:
	VkDevice m_device;
	VkBuffer m_buffer;
	VkDeviceMemory m_memory;
};