#pragma once
#include "Core/Renderer/CommandPool.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanCommandPool : public CommandPool {
public:
	using Ptr = Ref<VulkanCommandPool>;

	explicit VulkanCommandPool(VkDevice device);
	~VulkanCommandPool() override;

	void Create(const CommandPoolCreateInfo& createInfo) override;

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanCommandPool>(device);
	}

private:
	VkDevice m_device;
	VkCommandPool m_pool;
};