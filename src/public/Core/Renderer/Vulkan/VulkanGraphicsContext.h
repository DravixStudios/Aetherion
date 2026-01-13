#pragma once
#include "Core/Renderer/GraphicsContext.h"

#include "Core/Renderer/Vulkan/VulkanPipeline.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanGraphicsContext : public GraphicsContext {
public:
	using Ptr = Ref<VulkanGraphicsContext>;

	VulkanGraphicsContext(VkCommandBuffer commandBuffer);
	~VulkanGraphicsContext() override = default;

	void BindPipeline(Ref<Pipeline> pipeline) override;
	void BindDescriptorSets(
		uint32_t nFirstSet, 
		const Vector<Ref<DescriptorSet>>& sets, const Vector<uint32_t>& dynamicOffsets = {}) override;

private:
	VkCommandBuffer m_commandBuffer;
	VkPipeline m_currentPipeline;
	VkPipelineLayout m_currentPipelineLayout;
	VkPipelineBindPoint m_currentBindPoint;
};