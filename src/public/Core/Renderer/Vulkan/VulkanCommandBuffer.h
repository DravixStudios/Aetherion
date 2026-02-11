#pragma once
#include "Core/Renderer/CommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanCommandBuffer : public CommandBuffer {
public:
	using Ptr = Ref<VulkanCommandBuffer>;
	
	VulkanCommandBuffer(VkDevice device, VkCommandBuffer buffer);
	~VulkanCommandBuffer() override;

	void Begin() override;
	void End() override;
	void Reset() override;

	VkCommandBuffer GetVkCommandBuffer() const { return this->m_commandBuffer; }

	static Ptr
	CreateShared(VkDevice device, VkCommandBuffer buffer) {
		return CreateRef<VulkanCommandBuffer>(device, buffer);
	}
private:
	VkDevice m_device;
	
	VkCommandBuffer m_commandBuffer;
};