#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPURingBuffer.h"
#include "Core/Renderer/GraphicsContext.h"

class SkyAtmosphere {
public:
	~SkyAtmosphere() = default;

	void Init(Ref<Device> device, uint32_t nFramesInFlight);
	void Update(Ref<GraphicsContext> context, uint32_t nFrameIdx);

private:
	Ref<Device> m_device;
	uint32_t m_nFramesInFlight = 0;

	void CreateResources();
	void CreateDescriptors();	
};