#pragma once
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanRenderPass : public RenderPass {
public:
	using Ptr = Ref<VulkanRenderPass>;

	explicit VulkanRenderPass(VkDevice device);
	~VulkanRenderPass() override;

	void Create(const RenderPassCreateInfo& createInfo) override;

	VkRenderPass GetVkRenderPass()  const { return this->m_renderPass; }
	
	static Ptr CreateShared(VkDevice device) {
		return CreateRef<VulkanRenderPass>(device);
	}
private:
	VkDevice m_device;
	VkRenderPass m_renderPass;
};