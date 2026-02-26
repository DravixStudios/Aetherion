#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/GPURingBuffer.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/PipelineLayout.h"

static constexpr uint32_t SKYBOX_FACE_SIZE = 512;

class SkyAtmosphere {
public:
	~SkyAtmosphere() = default;

	void Init(Ref<Device> device, uint32_t nFramesInFlight);
	void Update(Ref<GraphicsContext> context, uint32_t nFrameIdx);

	void SetSunDirection(const glm::vec3& sunDir);
	void SetViewProjection(const glm::mat4& view, const glm::mat4& proj);

	Ref<GPUTexture> GetSkyboxTexture() const { return m_skybox; }
	Ref<ImageView> GetSkyboxView() const { return m_skyboxView; }

private:
	struct CameraData {
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct SunData {
		glm::vec3 sunDir;
		float sunHeight;
	};

	Ref<Device> m_device;
	uint32_t m_nFramesInFlight = 0;

	glm::vec3 m_sunDir = glm::vec3(0.f, 1.f, 0.f);

	glm::mat4 m_view = glm::mat4(1.f);
	glm::mat4 m_proj = glm::mat4(1.f);

	Ref<GPUBuffer> m_cubeVBO;
	Ref<GPUBuffer> m_cubeIBO;
	uint32_t m_nIndexCount = 0;

	Ref<GPURingBuffer> m_sunDataBuff;
	Ref<GPURingBuffer> m_camDataBuff;

	Ref<DescriptorSet> m_camSet;
	Ref<DescriptorSetLayout> m_camSetLayout;
	Ref<DescriptorPool> m_camPool;

	Ref<DescriptorSet> m_sunSet;
	Ref<DescriptorSetLayout> m_sunSetLayout;
	Ref<DescriptorPool> m_sunPool;

	Ref<Pipeline> m_pipeline;
	Ref<PipelineLayout> m_pipelineLayout;

	Ref<GPUTexture> m_skybox;
	Ref<ImageView> m_skyboxView;

	Ref<RenderPass> m_renderPass;
	Vector<Ref<Framebuffer>> m_framebuffers;
	Vector<Ref<ImageView>> m_imageViews;

	void CreateResources();
	void CreateDescriptors();
	void CreatePipeline();
};