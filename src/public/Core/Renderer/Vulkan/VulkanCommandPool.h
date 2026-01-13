#pragma once
#include "Core/Renderer/CommandPool.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanCommandPool : public CommandPool {
public:
	using Ptr = Ref<VulkanCommandPool>;

	explicit VulkanCommandPool(VkDevice device);
	~VulkanCommandPool() override;

	void Create(const CommandPoolCreateInfo& createInfo) override;

	Ref<CommandBuffer> AllocateCommandBuffer() override;
	Vector<Ref<CommandBuffer>> AllocateCommandBuffers(uint32_t nCount) override;

	void FreeCommandBuffer(Ref<CommandBuffer> commandBuffer) override;
	void FreeCommandBuffers(Vector<Ref<CommandBuffer>> commandBuffers) override;;

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanCommandPool>(device);
	}

private:
	VkDevice m_device;
	VkCommandPool m_pool;

	VkCommandPoolCreateFlags ConvertCommandPoolFlags(ECommandPoolFlags flags);
};