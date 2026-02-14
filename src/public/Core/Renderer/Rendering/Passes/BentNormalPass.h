#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"

class BentNormalPass : public BasePass {
public:
	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);
	void SetupNode(RenderGraphBuilder& builder);
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nImgIdx);
	void SetInput(TextureHandle normal, TextureHandle depth);
	void SetOutput(TextureHandle bentNormal);

	void SetCameraData(glm::mat4 view, glm::mat4 projection);

	TextureHandle GetOutput() const { return this->m_output; }

	void SetScreenQuad(Ref<GPUBuffer> sqVBO, Ref<GPUBuffer> sqIBO, uint32_t nIndexCount);
private:
	struct CameraData {
		glm::mat4 inverseView;
		glm::mat4 inverseProjection;
		glm::mat4 projection;
		float radius = .5f;
	};

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

	Vector<Ref<DescriptorSet>> m_descriptorSets;
	Ref<DescriptorSetLayout> m_setLayout;
	Ref<DescriptorPool> m_pool;

	Ref<DescriptorSet> m_cameraSet;
	Ref<DescriptorSetLayout> m_cameraSetLayout;
	Ref<DescriptorPool> m_cameraPool;

	Ref<GPURingBuffer> m_cameraBuff;

	Ref<RenderPass> m_renderPass;

	Ref<GPUBuffer> m_sqVBO;
	Ref<GPUBuffer> m_sqIBO;
	uint32_t m_nIndexCount = 0;

	uint32_t m_nFramesInFlight;

	glm::mat4 m_view = glm::mat4(1.f);
	glm::mat4 m_projection = glm::mat4(1.f);
};