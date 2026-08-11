#pragma once
#include "Core/Renderer/CommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice;

class VulkanCommandBuffer : public CommandBuffer {
public:
	using Ptr = Ref<VulkanCommandBuffer>;
	
	VulkanCommandBuffer(Ref<VulkanDevice> device, VkCommandBuffer buffer);
	~VulkanCommandBuffer() override;

	void Begin(bool bSingleTime = false) override;
	void End() override;
	void Reset() override;

	VkCommandBuffer GetVkCommandBuffer() const { return this->m_commandBuffer; }

	Ref<VulkanDevice> GetDevice() const { return this->m_device; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device, VkCommandBuffer buffer) {
		return CreateRef<VulkanCommandBuffer>(device, buffer);
	}
private:
	Ref<VulkanDevice> m_device;
	
	VkCommandBuffer m_commandBuffer;
};