#pragma once
#include "Core/Renderer/GraphicsContext.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
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

	void BindVertexBuffers(const Vector<Ref<GPUBuffer>>& buffers, const Vector<size_t>& offsets = {}) override;
	void BindIndexBuffer(Ref<GPUBuffer> buffer, EIndexType indexType = EIndexType::UINT16) override;

private:
	VkCommandBuffer m_commandBuffer;
	VkPipeline m_currentPipeline;
	VkPipelineLayout m_currentPipelineLayout;
	VkPipelineBindPoint m_currentBindPoint;
};