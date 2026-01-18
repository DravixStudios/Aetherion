#include "Core/Renderer/Vulkan/VulkanBuffer.h"

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
* Creates a Vulkan buffer
* 
* @param pcData Constant pointer to the buffer data
* @param nSize Size of the data
* @param bufferType Type of buffer
*/
void VulkanBuffer::Create(const void* pcData, uint32_t nSize, EBufferType bufferType) {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	/* Buffer creation */
	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = nSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VulkanHelpers::ConvertBufferUsage(bufferType);

	VK_CHECK(vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &this->m_buffer), "Failed creating a buffer");

	/* Query buffer memory requirements */
	VkMemoryRequirements memReqs = { };
	vkGetBufferMemoryRequirements(vkDevice, this->m_buffer, &memReqs);

	/* Allocate memory */
	VkDeviceMemory vkMemory = { };
	
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->m_device->FindMemoryType(
		memReqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	VK_CHECK(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &this->m_memory), "Failed allocating buffer memory");
	VK_CHECK(vkBindBufferMemory(vkDevice, this->m_buffer, this->m_memory, 0), "Failed binding buffer memory");

	/* Copy data to buffer */
	void* pMap = nullptr;
	vkMapMemory(vkDevice, this->m_memory, 0, VK_WHOLE_SIZE, 0, &pMap);
	memcpy(pMap, pcData, nSize);
	vkUnmapMemory(vkDevice, this->m_memory);
	pMap = nullptr;
}