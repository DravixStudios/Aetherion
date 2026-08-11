#pragma once
#include "Core/Renderer/Device.h"

#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/PipelineLayout.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/Framebuffer.h"

class IBLGenerator {
	static constexpr uint32_t IRRADIANCE_SIZE = 64;
	static constexpr uint32_t PREFILTER_SIZE = 128;
	static constexpr uint32_t BRDF_SIZE = 512;
	static constexpr uint32_t MIP_LEVELS = 5;
	static constexpr GPUFormat CUBEMAP_FORMAT = GPUFormat::RGBA32_FLOAT;
	static constexpr GPUFormat BRDF_FORMAT = GPUFormat::RG16_FLOAT;
public:
	void Init(
		Ref<Device> device, 
		Ref<GPUTexture> skybox, 
		Ref<ImageView> skyboxView,
		Ref<Sampler> sampler,
		Ref<GPUBuffer> sqVBO,
		Ref<GPUBuffer> sqIBO,
		uint32_t nFramesInFlight = 1
	);

	void Generate(Ref<GraphicsContext> context, uint32_t nFrameIdx = 0);

	Ref<GPUTexture> GetIrradiance() const { return this->m_irradiance; }
	Ref<GPUTexture> GetPrefilter() const { return this->m_prefilter; }
	Ref<GPUTexture> GetBRDF() const { return this->m_brdf; }

	Ref<ImageView> GetIrradianceView() const { return this->m_irradianceView; }
	Ref<ImageView> GetPrefilterView() const { return this->m_prefilterView; }
	Ref<ImageView> GetBRDFView() const { return this->m_brdfView; }
private:
	void CreateDescriptors();
	void CreateRenderPasses();
	void CreatePipelines();
	void CreateResources();

	struct ViewProjection {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	Ref<Device> m_device;
	uint32_t m_nFramesInFlight = 1;

	Ref<RenderPass> m_irradianceRP;
	Ref<RenderPass> m_prefilterRP;
	Ref<RenderPass> m_brdfRP;

	Ref<GPUTexture> m_irradiance;
	Ref<GPUTexture> m_prefilter;
	Ref<GPUTexture> m_brdf;

	Ref<ImageView> m_irradianceView;
	Ref<ImageView> m_prefilterView;
	Ref<ImageView> m_brdfView;

	Ref<DescriptorSet> m_irradianceSet;
	Ref<DescriptorSet> m_prefilterSet;

	Ref<DescriptorSetLayout> m_irradianceSetLayout;
	Ref<DescriptorSetLayout> m_prefilterSetLayout;

	Ref<DescriptorPool> m_irradiancePool;
	Ref<DescriptorPool> m_prefilterPool;

	Ref<PipelineLayout> m_irradianceLayout;
	Ref<PipelineLayout> m_prefilterLayout;
	Ref<PipelineLayout> m_brdfLayout;

	Ref<Pipeline> m_irradiancePipeline;
	Ref<Pipeline> m_prefilterPipeline;
	Ref<Pipeline> m_brdfPipeline;

	Ref<GPUTexture> m_skybox;
	Ref<ImageView> m_skyboxView;
	Ref<Sampler> m_sampler;

	Ref<GPUBuffer> m_cubeVBO;
	Ref<GPUBuffer> m_cubeIBO;

	Ref<GPUBuffer> m_sqVBO;
	Ref<GPUBuffer> m_sqIBO;

	uint32_t m_nIndexCount = 0;

	/* Per-face framebuffers for irradiance (6) */
	Vector<Ref<Framebuffer>> m_irradianceFBs;
	Vector<Ref<ImageView>> m_irradianceFaceViews;

	/* Per-face, per-mip framebuffers for prefilter (6 * MIP_LEVELS) */
	Vector<Vector<Ref<Framebuffer>>> m_prefilterFBs;
	Vector<Vector<Ref<ImageView>>> m_prefilterFaceViews;

	/* BRDF framebuffer */
	Ref<Framebuffer> m_brdfFB;
	Ref<ImageView> m_brdfFaceView;

	Ref<GPURingBuffer> m_viewProjBuffer;
	Ref<DescriptorSet> m_viewProjSet;
	Ref<DescriptorSetLayout> m_viewProjSetLayout;
	Ref<DescriptorPool> m_viewProjPool;
};