#pragma once
#include "Core/Renderer/Fence.h"

#include "Core/Renderer/Vulkan/VulkanDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanFence : public Fence {
public:
	using Ptr = Ref<VulkanFence>;

	explicit VulkanFence(Ref<VulkanDevice> device);
	~VulkanFence() override;

	void Create(const FenceCreateInfo& createInfo) override;

	void Reset() override;

	VkFence GetVkFence() const { return this->m_fence; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanFence>(device);
	}

private:
	Ref<VulkanDevice> m_device;

	VkFence m_fence;
};