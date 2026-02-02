#include "Core/Renderer/Vulkan/VulkanRingBuffer.h"

/**
* Converts EBufferType to VkBufferUsageFlagBits
* 
* @param bufferType Buffer type
* 
* @returns Vulkan buffer usage
*/
VkBufferUsageFlagBits ConvertTypeToUsage(EBufferType bufferType) {
	switch (bufferType) {
	case EBufferType::CONSTANT_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	case EBufferType::VERTEX_BUFFER: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	case EBufferType::INDEX_BUFFER: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	case EBufferType::STORAGE_BUFFER:
		return static_cast<VkBufferUsageFlagBits>(
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT  |
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT
		);
	default: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
}

VulkanRingBuffer::VulkanRingBuffer(Ref<VulkanDevice> device) 
	: m_device(device), 
	m_nPerFrameSize(0), m_nOffset(0),
	m_buffer(VK_NULL_HANDLE), m_memory(VK_NULL_HANDLE) {}

VulkanRingBuffer::~VulkanRingBuffer() {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	if (this->m_buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(vkDevice, this->m_buffer, nullptr);
	}

	if (this->m_memory != VK_NULL_HANDLE) {
		vkUnmapMemory(vkDevice, this->m_memory);
		vkFreeMemory(vkDevice, this->m_memory, nullptr);
	}
}

/**
* Creates a ring buffer
* 
* @param createInfo Ring Buffer create info
*/
void 
VulkanRingBuffer::Create(const RingBufferCreateInfo& createInfo) {
	this->m_bufferType = createInfo.bufferType;
	this->m_nBufferSize = createInfo.nBufferSize;
	this->m_nFramesInFlight = createInfo.nFramesInFlight;

	VkPhysicalDeviceProperties devProps = this->m_device->GetPhysicalDeviceProperties();
	uint32_t nMinAlignment = createInfo.nAlignment;

	/* Different alignment depending on the buffer type */
	switch (this->m_bufferType) {
		case EBufferType::CONSTANT_BUFFER:
			nMinAlignment = std::max(static_cast<VkDeviceSize>(createInfo.nAlignment), devProps.limits.minUniformBufferOffsetAlignment);
			break;
		case EBufferType::STORAGE_BUFFER:
			nMinAlignment = std::max(static_cast<VkDeviceSize>(createInfo.nAlignment), devProps.limits.minStorageBufferOffsetAlignment);
			break;
		case EBufferType::VERTEX_BUFFER:
		case EBufferType::INDEX_BUFFER:
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
	VkDevice vkDevice = this->m_device->GetVkDevice();

	VkBufferUsageFlagBits usage = ConvertTypeToUsage(createInfo.bufferType);

	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = createInfo.nBufferSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = usage;

	VK_CHECK(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &this->m_buffer), "Failed creating buffer");

	/* Allocate buffer memory */
	VkMemoryRequirements memReqs = { };
	vkGetBufferMemoryRequirements(vkDevice, this->m_buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->m_device->FindMemoryType(
		memReqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	VK_CHECK(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &this->m_memory), "Failed allocating buffer memory");

	VK_CHECK(vkBindBufferMemory(vkDevice, this->m_buffer, this->m_memory, 0), "Failed binding buffer memory");

	/* Map the memory persistently */
	vkMapMemory(vkDevice, this->m_memory, 0, VK_WHOLE_SIZE, 0, &this->pMap);

	String bufferTypeName = "UNKNOWN";
	switch (this->m_bufferType) {
		case EBufferType::CONSTANT_BUFFER: bufferTypeName = "CONSTANT"; break;
		case EBufferType::VERTEX_BUFFER: bufferTypeName = "VERTEX"; break;
		case EBufferType::INDEX_BUFFER: bufferTypeName = "INDEX"; break;
		case EBufferType::STORAGE_BUFFER: bufferTypeName = "STORAGE"; break;
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