#pragma once
#include "Core/Renderer/CommandPool.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice;

class VulkanCommandPool : public CommandPool {
public:
	using Ptr = Ref<VulkanCommandPool>;

	explicit VulkanCommandPool(Ref<VulkanDevice> device);
	~VulkanCommandPool() override;

	void Create(const CommandPoolCreateInfo& createInfo) override;

	Ref<CommandBuffer> AllocateCommandBuffer() override;
	Vector<Ref<CommandBuffer>> AllocateCommandBuffers(uint32_t nCount) override;

	void FreeCommandBuffer(Ref<CommandBuffer> commandBuffer) override;
	void FreeCommandBuffers(const Vector<Ref<CommandBuffer>>& commandBuffers) override;

	void Reset(bool bReleaseResources) override;

	VkCommandPool GetVkCommandPool() const { return this->m_pool; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanCommandPool>(device);
	}

private:
	Ref<VulkanDevice> m_device;
	VkCommandPool m_pool;

	VkCommandPoolCreateFlags ConvertCommandPoolFlags(ECommandPoolFlags flags);
};