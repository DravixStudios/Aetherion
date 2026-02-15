#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/Sampler.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/Shader.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/DescriptorSetLayout.h"

class SunExtraction {
public:
	void Init(Ref<Device>, Ref<GPUTexture> skybox, Ref<ImageView> skyboxView, Ref<Sampler> cubeSampler);

	void Extract(Ref<GraphicsContext> context);

	Ref<GPUBuffer> GetSunResult() const { return this->m_sunResultBuff; }
private:
	Ref<Device> m_device;

	Ref<Pipeline> m_pipeline;
	Ref<PipelineLayout> m_pipelineLayout;

	Ref<DescriptorSet> m_descriptorSet;
	Ref<DescriptorSetLayout> m_setLayout;
	Ref<DescriptorPool> m_pool;

	Ref<GPUBuffer> m_sunResultBuff;

	Ref<Sampler> m_sampler;
	Ref<GPUTexture> m_skybox;
	Ref<ImageView> m_skyboxView;

	void CreateDescriptors();
	void CreatePipeline();
};