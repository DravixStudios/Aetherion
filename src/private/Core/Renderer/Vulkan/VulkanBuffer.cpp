#include "Core/Renderer/Vulkan/VulkanBuffer.h"

/**
* (EBufferUsage -> VkBufferUsageFlags)
* 
* @param usage Buffer usage
* 
* @returns Vulkan buffer usage
*/
VkBufferUsageFlags
ConvertBufferUsage(EBufferUsage usage) {
	VkBufferUsageFlags vkUsage = 0;

	if ((usage & EBufferUsage::TRANSFER_SRC) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	if ((usage & EBufferUsage::TRANSFER_DST) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if ((usage & EBufferUsage::VERTEX_BUFFER) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if ((usage & EBufferUsage::INDEX_BUFFER) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if ((usage & EBufferUsage::UNIFORM_BUFFER) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}

	if ((usage & EBufferUsage::STORAGE_BUFFER) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	if ((usage & EBufferUsage::INDIRECT_BUFFER) != EBufferUsage::NONE) {
		vkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}

	return vkUsage;
}

/**
* (EBufferCreateFlags -> VkBufferCreateFlags)
* 
* @param flags Buffer create flags
* 
* @returns Vulkan buffer create flags
*/
VkBufferCreateFlags
ConvertBufferFlags(EBufferCreateFlags flags) {
	VkBufferCreateFlags vkFlags = 0;

	if ((flags & EBufferCreateFlags::SPARSE_BINDING) != static_cast<EBufferCreateFlags>(0)) {
		vkFlags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
	}

	if ((flags & EBufferCreateFlags::SPARSE_RESIDENCY) != static_cast<EBufferCreateFlags>(0)) {
		vkFlags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
	}

	if ((flags & EBufferCreateFlags::SPARSE_ALIASED) != static_cast<EBufferCreateFlags>(0)) {
		vkFlags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
	}

	if ((flags & EBufferCreateFlags::PROTECTED) != static_cast<EBufferCreateFlags>(0)) {
		vkFlags |= VK_BUFFER_CREATE_PROTECTED_BIT;
	}

	return vkFlags;
}

VulkanBuffer::VulkanBuffer(Ref<VulkanDevice> device) 
	: m_device(device), m_buffer(VK_NULL_HANDLE), m_memory(VK_NULL_HANDLE) {}

VulkanBuffer::~VulkanBuffer() {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	if (this->m_memory) {
		vkFreeMemory(vkDevice, this->m_memory, nullptr);
	}

	if (this->m_buffer) {
		vkDestroyBuffer(vkDevice, this->m_buffer, nullptr);
	}
}

/**
* Creates the Vulkan buffer
* 
* @param createInfo Buffer create info
*/
void 
VulkanBuffer::Create(const BufferCreateInfo& createInfo) {
	VkDevice vkDevice = this->m_device->GetVkDevice();
	this->m_bufferType = createInfo.type;

	this->m_size = static_cast<VkDeviceSize>(createInfo.nSize);

	/* Buffer creation */
	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = this->m_size;
	bufferInfo.sharingMode = VulkanHelpers::ConvertSharingMode(createInfo.sharingMode);
	bufferInfo.usage = ConvertBufferUsage(createInfo.usage);
	bufferInfo.flags = ConvertBufferFlags(createInfo.flags);

	VK_CHECK(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &this->m_buffer), "Failed creating a buffer");

	/* Query buffer memory requirements */
	VkMemoryRequirements memReqs = { };
	vkGetBufferMemoryRequirements(vkDevice, this->m_buffer, &memReqs);

	/* Allocate memory */
	VkDeviceMemory vkMemory = { };

	/*
		Select memory properties
		depending on the buffer type

		TODO: Contemplate CPU non-visible
		storage buffers
	*/
	VkMemoryPropertyFlags memProps = 0;
	switch (createInfo.type) {
	case EBufferType::VERTEX_BUFFER:
	case EBufferType::INDEX_BUFFER:
		memProps |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		break;
	case EBufferType::STAGING_BUFFER:
		memProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		memProps |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		memProps |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		break;
	case EBufferType::UNIFORM_BUFFER:
	case EBufferType::STORAGE_BUFFER:
		memProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		memProps |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		break;
	default:
		Logger::Debug("VulkanBuffer::Create: Unknown buffer type");
		return;
	}
	
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->m_device->FindMemoryType(memReqs.memoryTypeBits, memProps);

	VK_CHECK(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &this->m_memory), "Failed allocating buffer memory");
	VK_CHECK(vkBindBufferMemory(vkDevice, this->m_buffer, this->m_memory, 0), "Failed binding buffer memory");

	/* Copy data to buffer */
	void* pMap = nullptr;
	if (createInfo.pcData != nullptr && createInfo.nSize > 0) {
		vkMapMemory(vkDevice, this->m_memory, 0, this->m_size, 0, &pMap);
		
		memcpy(pMap, createInfo.pcData, createInfo.nSize);

		vkUnmapMemory(vkDevice, this->m_memory);
	}
	
	pMap = nullptr;
}

/**
* Maps the buffer to CPU
*
* @returns A pointer to the map
*/
void* 
VulkanBuffer::Map() {
	void* pMap = nullptr;

	vkMapMemory(this->m_device->GetVkDevice(), this->m_memory, 0, this->m_size, 0, &pMap);

	return pMap;
}

/**
* Unmaps the buffer
*/
void
VulkanBuffer::Unmap() {
	vkUnmapMemory(this->m_device->GetVkDevice(), this->m_memory);
}