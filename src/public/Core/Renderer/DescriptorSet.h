#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/Sampler.h"

struct DescriptorBufferInfo {
	Ref<GPUBuffer> buffer;
	uint32_t nOffset = 0;
	uint32_t nRange = 0; // 0 = whole size (VK_WHOLE_SIZE)
};

struct DescriptorImageInfo {
	Ref<GPUTexture> texture;
	Ref<ImageView> imageView;
	Ref<Sampler> sampler;
};

class DescriptorSet {
public:
	static constexpr const char* CLASS_NAME = "DescriptorSet";

	using Ptr = Ref<DescriptorSet>;

	DescriptorSet() = default;
	virtual ~DescriptorSet() = default;

	virtual void Allocate(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) = 0;

	virtual void WriteBuffer(
		uint32_t nBinding, 
		uint32_t nArrayElement, 
		const DescriptorBufferInfo& bufferInfo
	) = 0;

	virtual void WriteTexture(uint32_t nBinding, uint32_t nArrayElement, const DescriptorImageInfo& imageInfo) = 0;

	virtual void WriteBuffers(
		uint32_t nBinding, 
		uint32_t nFirstArrayElement, 
		const Vector<DescriptorBufferInfo>& bufferInfos,
		EBufferType bufferType = EBufferType::UNIFORM_BUFFER
	) = 0;

	virtual void WriteTextures(
		uint32_t nBinding, 
		uint32_t nFirstArrayElement, 
		const Vector<DescriptorImageInfo>& imageInfos
	) = 0;

	virtual void UpdateWrites() = 0;
};