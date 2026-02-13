#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"

class BentNormalPass : public BasePass {
public:
	void Init(Ref<Device> device);
	void SetupNode(RenderGraphBuilder& builder);
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIdx);
	void SetInput(TextureHandle normal, TextureHandle depth);

	TextureHandle GetOutput() const { return this->m_output; }

	void SetScreenQuad(Ref<GPUBuffer> sqVBO, Ref<GPUBuffer> sqIBO, uint32_t nIndexCount);
private:
	void CreatePipeline();
	
	void CreateDescriptors();

	struct Input {
		TextureHandle normal;
		TextureHandle depth;
	};

	Input m_input;

	TextureHandle m_output;

	Ref<Pipeline> m_pipeline;
	Ref<PipelineLayout> m_layout;
	Ref<Sampler> m_sampler;

	Ref<DescriptorSet> m_descriptorSet;
	Ref<DescriptorSetLayout> m_setLayout;
	Ref<DescriptorPool> m_pool;

	Ref<RenderPass> m_renderPass;

	Ref<GPUBuffer> m_sqVBO;
	Ref<GPUBuffer> m_sqIBO;
	uint32_t m_nIndexCount = 0;
};