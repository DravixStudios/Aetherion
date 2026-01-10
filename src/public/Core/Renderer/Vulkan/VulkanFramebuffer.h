#pragma once
#include "Core/Renderer/Framebuffer.h"

#include "Core/Renderer/Vulkan/VulkanTexture.h"
#include "Core/Renderer/Vulkan/VulkanRenderPass.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanFramebuffer : public Framebuffer {
public:
	using Ptr = Ref<VulkanFramebuffer>;

	explicit VulkanFramebuffer(VkDevice device);
	~VulkanFramebuffer() override;

	void Create(const FramebufferCreateInfo& createInfo) override;

	VkFramebuffer GetVkFramebuffer() const { return this->m_framebuffer; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanFramebuffer>(device);
	}

private:
	VkDevice m_device;
	VkFramebuffer m_framebuffer;
};