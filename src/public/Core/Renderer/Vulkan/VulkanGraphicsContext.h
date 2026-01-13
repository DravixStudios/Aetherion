#pragma once
#include "Core/Renderer/GraphicsContext.h"

#include "Core/Renderer/Vulkan/VulkanPipeline.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanGraphicsContext : public GraphicsContext {
public:
	using Ptr = Ref<VulkanGraphicsContext>;

	VulkanGraphicsContext(VkCommandBuffer commandBuffer);
	~VulkanGraphicsContext() override = default;

	void BindPipeline(Ref<Pipeline> pipeline) override;

private:
	VkCommandBuffer m_commandBuffer;
	VkPipeline m_currentPipeline;
	VkPipelineBindPoint m_currentBindPoint;
};