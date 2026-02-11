#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/Passes/GBufferPass.h"

class LightingPass : public BasePass {
public:
	struct Input {
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle orm;
		TextureHandle emissive;
		TextureHandle position;
	};

	struct Output {
		TextureHandle hdrOutput;
	};

	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex = 0) override;

	void SetInput(const GBufferPass::Output& gbOutput);
	void SetGBufferDescriptorSet(Ref<DescriptorSet> set);
	void SetLightData(
		Ref<ImageView> irradiance,
		Ref<ImageView> prefilter,
		Ref<ImageView> brdf,
		Ref<GPUBuffer> vertexBuffer,
		Ref<GPUBuffer> indexBuffer,
		Ref<Sampler> cubeSampler,
		Ref<Sampler> linearSampler,
		uint32_t nIndexCount
	);
	
	void SetCameraPosition(const glm::vec3& position);

	Output GetOutput() const { return this->m_output; }
private:
	Input m_input;
	Output m_output;
	Ref<DescriptorSet> m_gbufferSet;

	Ref<ImageView> m_irradiance;
	Ref<ImageView> m_prefilter;
	Ref<ImageView> m_brdf;

	Ref<DescriptorSetLayout> m_lightSetLayout;
	Ref<DescriptorPool> m_lightPool;
	Ref<DescriptorSet> m_lightSet;

	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;
	uint32_t m_nIndexCount = 0;

	Ref<Sampler> m_cubeSampler;
	Ref<Sampler> m_linearSampler;

	uint32_t m_nFramesInFlight = 0;
	glm::vec3 m_cameraPosition;

	void CreateDescriptorSet();
	void CreatePipeline();
};