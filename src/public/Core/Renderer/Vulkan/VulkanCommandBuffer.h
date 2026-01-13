#pragma once
#include "Core/Renderer/CommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanCommandBuffer : public CommandBuffer {
public:
	using Ptr = Ref<VulkanCommandBuffer>;
	
	VulkanCommandBuffer(VkDevice device, VkCommandPool pool);
	~VulkanCommandBuffer() override;

	void Begin() override;
	void End() override;
	void Reset() override;

	static Ptr
	CreateShared(VkDevice device, VkCommandPool pool) {
		return CreateRef<VulkanCommandBuffer>(device, pool);
	}
private:
	VkDevice m_device;
	
	VkCommandBuffer m_commandBuffer;
	VkCommandPool m_pool;
};