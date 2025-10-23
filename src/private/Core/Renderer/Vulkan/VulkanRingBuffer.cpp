#include "Core/Renderer/Vulkan/VulkanRingBuffer.h"
#include "Core/Renderer/Vulkan/VulkanRenderer.h"

VulkanRingBuffer::VulkanRingBuffer(VkDevice& device, VkPhysicalDevice& physicalDevice) : GPURingBuffer() {
	this->m_device = device;
	this->m_physicalDevice = physicalDevice;
	this->m_nPerFrameSize = 0;
	this->m_nOffset = 0;
	this->pMap = nullptr;
}

void VulkanRingBuffer::Init(uint32_t nBufferSize, uint32_t nAlignment, uint32_t nFramesInFlight) {
	GPURingBuffer::Init(nBufferSize, nAlignment, nFramesInFlight);

	VkPhysicalDeviceProperties devProps;
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &devProps);
	uint32_t nMinAlignment = devProps.limits.minUniformBufferOffsetAlignment;
	this->m_nAlignment = std::max(this->m_nAlignment, nMinAlignment);

	this->m_nPerFrameSize = nBufferSize / nFramesInFlight;

	VulkanRenderer* vkRenderer = dynamic_cast<VulkanRenderer*>(this->m_renderer);
	if (vkRenderer == nullptr) {
		spdlog::error("VulkanUniformRingBuffer::Init: Core renderer is not a VulkanRenderer");
		throw std::runtime_error("VulkanUniformRingBuffer::Init: Core renderer is not a VulkanRenderer");
	
		return;
	}
	this->m_vkRenderer = vkRenderer;
	
	GPUBuffer* pBuffer = this->m_vkRenderer->CreateBuffer(nullptr, nBufferSize, EBufferType::CONSTANT_BUFFER);
	VulkanBuffer* vkBuffer = dynamic_cast<VulkanBuffer*>(pBuffer);

	if (vkBuffer == nullptr) {
		spdlog::error("VulkanUniformRingBuffer::Init: Created a buffer but it doesn't appear to be VulkanBuffer");
		throw std::runtime_error("VulkanUniformRingBuffer::Init: Created a buffer but it doesn't appear to be VulkanBuffer");
		return;
	}

	this->m_buffer = vkBuffer->GetBuffer();
	this->m_memory = vkBuffer->GetMemory();

	/* 
		Map all the memory persistently 
		TODO: Upload batch
	*/
	vkMapMemory(this->m_device, this->m_memory, 0, VK_WHOLE_SIZE, 0, &this->pMap);
	spdlog::debug("VulkanUniformRingBuffeer::Init: Ring buffer initialized with {0} bytes and {1} alignment", nBufferSize, nAlignment);
}

/* Allocates a chunk of memory from the ring buffer with proper alignments */
void* VulkanRingBuffer::Allocate(uint32_t nDataSize, uint32_t& outOffset) {
	/* Convert our data size to a size aligned with VulkanRingBuffer::m_nAlignment */
	uint32_t nAlignedSize = this->Align(nDataSize, this->m_nAlignment);

	/* Calculate actual block limit */
	uint32_t nFrameBaseOffset = (this->m_nOffset / this->m_nPerFrameSize) * this->m_nPerFrameSize;
	uint32_t nFrameEnd = nFrameBaseOffset + this->m_nPerFrameSize;

	if (this->m_nOffset + nAlignedSize > nFrameEnd) {
		spdlog::warn("VulkanRingBuffer::Allocate: Overflow inside frame region, wrapping to frame base");
		this->m_nOffset = nFrameBaseOffset;
	}

	outOffset = this->m_nOffset;
	void* pPtr = static_cast<uint8_t*>(this->pMap) + this->m_nOffset;
	this->m_nOffset += nAlignedSize;

	return pPtr;
}

uint32_t VulkanRingBuffer::Align(uint32_t nValue, uint32_t nAlignment) {
	return (nValue + nAlignment - 1) & ~(nAlignment - 1);
}

void VulkanRingBuffer::Reset(uint32_t nImageIndex) {
	this->m_nOffset = this->Align(this->m_nPerFrameSize * nImageIndex, this->m_nAlignment);
}

VkBuffer VulkanRingBuffer::GetBuffer() {
	return this->m_buffer;
}