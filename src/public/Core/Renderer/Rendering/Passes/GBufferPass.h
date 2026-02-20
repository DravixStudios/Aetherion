#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferManager.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/Shader.h"
#include "Core/Renderer/MegaBuffer.h"

class RenderGraph;

class GBufferPass : public BasePass {
public:
	struct Output {
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle orm;
		TextureHandle emissive;
		TextureHandle position;
		TextureHandle depth;
		TextureHandle bentNormal;
	};

	void Init(Ref<Device> device) override;
	void Resize(uint32_t nWidth, uint32_t nHeight);
	void ImportResources(RenderGraph& graph);
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex = 0) override;
	
	void SetSceneData(
		Ref<DescriptorSet> sceneSet,
		Ref<DescriptorSetLayout> sceneSetLayout,
		Ref<DescriptorSet> bindlessSet,
		Ref<DescriptorSetLayout> bindlessSetLayout,
		const Vector<MegaBuffer::Block>& blocks,
		uint32_t nBlockCount,
		Ref<GPUBuffer> countBuffer,
		Ref<GPURingBuffer> indirectBuffer,
		uint32_t nIndirectOffset,
		uint32_t nTotalBatches,
		uint32_t nMaxBatchesPerBlock
	);

	Ref<DescriptorSet> GetReadDescriptorSet() const { return this->m_gbuffer.GetReadDescriptorSet(); }
	Ref<ImageView> GetDepthView() const { return this->m_gbuffer.GetDepthView(); }
	Output GetOutput() const { return this->m_output; }
private:
	GBufferManager m_gbuffer;
	Ref<RenderPass> m_compatRenderPass;

	Output m_output;
	Ref<DescriptorSet> m_sceneSet;
	Ref<DescriptorSetLayout> m_sceneSetLayout;

	Ref<DescriptorSet> m_bindlessSet;
	Ref<DescriptorSetLayout> m_bindlessSetLayout;

	Vector<MegaBuffer::Block> m_blocks;
	uint32_t m_nBlockCount = 0;

	Ref<GPUBuffer> m_countBuffer;
	Ref<GPURingBuffer> m_indirectBuffer;

	uint32_t m_nIndirectOffset = 0;
	uint32_t m_nTotalBatches = 0;
	uint32_t m_nMaxBatchesPerBlock = 0;

	void CreatePipeline();
};