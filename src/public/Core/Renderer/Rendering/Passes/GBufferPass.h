#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferManager.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/Shader.h"

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
		Ref<GPUBuffer> vertexBuffer,
		Ref<GPUBuffer> indexBuffer,
		uint32_t nIndexCount,
		Ref<GPUBuffer> countBuffer,
		Ref<GPUBuffer> indirectBuffer
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

	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;
	uint32_t m_nIndexCount = 0;

	Ref<GPUBuffer> m_countBuffer;
	Ref<GPUBuffer> m_indirectBuffer;

	void CreatePipeline();
};