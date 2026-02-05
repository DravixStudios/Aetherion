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

	void Draw(
		uint32_t nVertexCount,
		uint32_t nInstanceCount = 1,
		uint32_t nFirstVertex = 0,
		uint32_t nFirstInstance = 0
	) override;

	void DrawIndexed(
		uint32_t nIndexCount,
		uint32_t nInstanceCount = 1,
		uint32_t nFirstIndex = 0,
		uint32_t nVertexOffset = 0,
		uint32_t nFirstInstance = 0
	) override;

	void DrawIndexedIndirect(
		Ref<GPUBuffer> buffer,
		uint32_t nOffset,
		Ref<GPUBuffer> countBuffer,
		uint32_t nCountBufferOffset,
		uint32_t nMaxDrawCount,
		uint32_t nStride = 0
	) override;

	void PushConstants(
		Ref<PipelineLayout> layout,
		EShaderStage stages,
		uint32_t nOffsets,
		uint32_t nSize,
		const void* pcData
	) override;

	void SetViewport(const Viewport& viewport) override;
	void SetScissor(const Rect2D& scissor) override;

	void BeginRenderPass(const RenderPassBeginInfo& beginInfo) override;
	void EndRenderPass() override;
	void NextSubpass() override;

	void FillBuffer(Ref<GPUBuffer> buffer, uint32_t nOffset, uint32_t nSize, uint32_t nData) override;

	void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

	static Ptr
	CreateShared(VkCommandBuffer commandBuffer) {
		return CreateRef<VulkanGraphicsContext>(commandBuffer);
	}
private:
	VkCommandBuffer m_commandBuffer;
	VkPipeline m_currentPipeline;
	VkPipelineLayout m_currentPipelineLayout;
	VkPipelineBindPoint m_currentBindPoint;
};