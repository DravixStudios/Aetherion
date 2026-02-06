#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "Core/Renderer/GPUBuffer.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Core/Renderer/Vulkan/VulkanDevice.h"

class VulkanBuffer : public GPUBuffer {
public:
	using Ptr = Ref<VulkanBuffer>;

	explicit VulkanBuffer(Ref<VulkanDevice> device);
	~VulkanBuffer() override;

	void Create(const BufferCreateInfo& createInfo) override;

	VkBuffer GetVkBuffer() const { return this->m_buffer; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanBuffer>(device);
	}

private:
	Ref<VulkanDevice> m_device;
	VkBuffer m_buffer;
	VkDeviceMemory m_memory;

	VkDeviceSize m_size = 0;
};