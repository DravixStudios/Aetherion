#pragma once
#include "Core/Renderer/GPURingBuffer.h"
#include "Core/Containers.h"

#include "Core/Renderer/Vulkan/VulkanDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>

class VulkanRingBuffer : public GPURingBuffer {
public:
	using Ptr = Ref<VulkanRingBuffer>;

	explicit VulkanRingBuffer(Ref<VulkanDevice> device);
	~VulkanRingBuffer() override;

	void Create(const RingBufferCreateInfo& createInfo) override;
	void* Allocate(uint32_t nDataSize, uint32_t& outOffset) override;
	uint32_t Align(uint32_t nValue, uint32_t nAlignment) override;
	void Reset(uint32_t nImageIndex) override;

	uint32_t GetPerFrameSize() const { return this->m_nPerFrameSize; }
	uint32_t GetOffset() const { return this->m_nOffset; }

	uint64_t GetSize() const override { return this->m_nBufferSize; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanRingBuffer>(device);
	}
private:
	Ref<VulkanDevice> m_device;

	uint32_t m_nPerFrameSize;
	uint32_t m_nOffset;

	void* pMap;
};