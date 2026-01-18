#include "Core/Renderer/Vulkan/VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(VkDevice device) 
	: m_device(device), m_buffer(VK_NULL_HANDLE), m_memory(VK_NULL_HANDLE) {}

VulkanBuffer::~VulkanBuffer() {
	if (this->m_memory) {
		vkFreeMemory(this->m_device, this->m_memory, nullptr);
	}

	if (this->m_buffer) {
		vkDestroyBuffer(this->m_device, this->m_buffer, nullptr);
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
	VkBuffer vkBuffer = nullptr;

	/* Buffer creation */
	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = nSize;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.usage = VulkanHelpers::ConvertBufferUsage(bufferType);

	VK_CHECK(vkCreateBuffer(this->m_device, &bufferInfo, nullptr, &vkBuffer), "Failed creating a buffer");

	/* Query buffer memory requirements */
	VkMemoryRequirements memReqs = { };
	vkGetBufferMemoryRequirements(this->m_device, vkBuffer, &memReqs);

	/* Allocate memory */
	VkDeviceMemory vkMemory = { };
	
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
}