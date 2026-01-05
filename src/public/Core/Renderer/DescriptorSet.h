#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/DescriptorSetLayout.h"

struct DescriptorBufferInfo {
	Ref<GPUBuffer> buffer;
	uint32_t nOffset = 0;
	uint32_t nRange = 0; // 0 = whole size (VK_WHOLE_SIZE)
};

/* 
	The texture in this struct, is for special cases
	where a separate sampler is needed.

	Normally the texture comes with it's sampler
*/
struct DescriptorImageInfo {
	Ref<GPUTexture> texture;
};

class DescriptorSet {
public:
	using Ptr = Ref<DescriptorSet>;

	DescriptorSet() = default;
	virtual ~DescriptorSet() = default;

	virtual void Allocate(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) = 0;

	virtual void WriteBuffer(uint32_t nBinding, uint32_t nArrayElement, const DescriptorBufferInfo& bufferInfo) = 0;
	virtual void WriteTexture(uint32_t nBinding, uint32_t nArrayElement, const DescriptorImageInfo& imageInfo) = 0;

	virtual void UpdateWrites() = 0;
};