#include "Core/Renderer/Vulkan/VulkanRingBuffer.h"

EBufferType
ConvertUsageToType(EBufferUsage usage){
	if ((usage & EBufferUsage::VERTEX_BUFFER) != static_cast<EBufferUsage>(0))
		return EBufferType::VERTEX_BUFFER;

	if ((usage & EBufferUsage::INDEX_BUFFER) != static_cast<EBufferUsage>(0))
		return EBufferType::INDEX_BUFFER;

	if ((usage & EBufferUsage::UNIFORM_BUFFER) != static_cast<EBufferUsage>(0))
		return EBufferType::UNIFORM_BUFFER;

	if ((usage & EBufferUsage::STORAGE_BUFFER) != static_cast<EBufferUsage>(0))
		return EBufferType::STORAGE_BUFFER;

	if (((usage & EBufferUsage::TRANSFER_SRC) != static_cast<EBufferUsage>(0)) ||
		((usage & EBufferUsage::TRANSFER_DST) != static_cast<EBufferUsage>(0)))
		return EBufferType::STAGING_BUFFER;

	return EBufferType::UNKNOWN_BUFFER;
}

VulkanRingBuffer::VulkanRingBuffer(Ref<VulkanDevice> device) 
	: m_device(device), 
	m_nPerFrameSize(0), m_nOffset(0) {}

VulkanRingBuffer::~VulkanRingBuffer() {
	this->m_buffer->Unmap();
	this->pMap = nullptr;
}

/**
* Creates a ring buffer
* 
* @param createInfo Ring Buffer create info
*/
void 
VulkanRingBuffer::Create(const RingBufferCreateInfo& createInfo) {
	this->m_usage = createInfo.usage;
	this->m_nBufferSize = createInfo.nBufferSize;
	this->m_nFramesInFlight = createInfo.nFramesInFlight;

	VkPhysicalDeviceProperties devProps = this->m_device->GetPhysicalDeviceProperties();
	uint32_t nMinAlignment = createInfo.nAlignment;

	/* Different alignment depending on the buffer type */
	switch (this->m_usage) {
		case EBufferUsage::UNIFORM_BUFFER:
			nMinAlignment = std::max(static_cast<VkDeviceSize>(createInfo.nAlignment), devProps.limits.minUniformBufferOffsetAlignment);
			break;
		case EBufferUsage::STORAGE_BUFFER:
			nMinAlignment = std::max(static_cast<VkDeviceSize>(createInfo.nAlignment), devProps.limits.minStorageBufferOffsetAlignment);
			break;
		case EBufferUsage::VERTEX_BUFFER:
		case EBufferUsage::INDEX_BUFFER:
			nMinAlignment = createInfo.nAlignment;
			break;
		default:
			nMinAlignment = createInfo.nAlignment;
			break;
	}

	/* Calculate the alignment and per frame size */
	this->m_nAlignment = std::max(createInfo.nAlignment, nMinAlignment);
	this->m_nPerFrameSize = createInfo.nBufferSize / createInfo.nFramesInFlight;

	/* Buffer creation */
	BufferCreateInfo bufferInfo = { };
	bufferInfo.nSize = createInfo.nBufferSize;
	bufferInfo.usage = createInfo.usage;
	bufferInfo.sharingMode = ESharingMode::EXCLUSIVE;
	bufferInfo.type = ConvertUsageToType(createInfo.usage);

	this->m_buffer = this->m_device->CreateBuffer(bufferInfo);

	/* Map the memory persistently */
	this->pMap = this->m_buffer->Map();

	String bufferTypeName = "UNKNOWN";
	switch (this->m_usage) {
		case EBufferUsage::UNIFORM_BUFFER: bufferTypeName = "CONSTANT"; break;
		case EBufferUsage::VERTEX_BUFFER: bufferTypeName = "VERTEX"; break;
		case EBufferUsage::INDEX_BUFFER: bufferTypeName = "INDEX"; break;
		case EBufferUsage::STORAGE_BUFFER: bufferTypeName = "STORAGE"; break;
	}

	Logger::Debug("VulkanUniformRingBuffeer::Init: {0} Ring buffer initialized", bufferTypeName);
	Logger::Debug("  - Total size: {0} KB", createInfo.nBufferSize / 1024);
	Logger::Debug("  - Per-frame size: {0} KB", this->m_nPerFrameSize / 1024);
	Logger::Debug("  - Alignment: {0} bytes", this->m_nAlignment);
	Logger::Debug("  - Frames in flight: {0}", this->m_nFramesInFlight);
}

/* Allocates a chunk of memory from the ring buffer with proper alignments */
void* 
VulkanRingBuffer::Allocate(uint32_t nDataSize, uint32_t& outOffset) {
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

uint32_t 
VulkanRingBuffer::Align(uint32_t nValue, uint32_t nAlignment) {
	return (nValue + nAlignment - 1) & ~(nAlignment - 1);
}

void 
VulkanRingBuffer::Reset(uint32_t nImageIndex) {
	this->m_nOffset = this->Align(this->m_nPerFrameSize * nImageIndex, this->m_nAlignment);
}