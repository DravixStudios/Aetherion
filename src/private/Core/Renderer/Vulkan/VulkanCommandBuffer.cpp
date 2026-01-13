#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device)
	: m_device(device), m_commandBuffer(VK_NULL_HANDLE) { }

VulkanCommandBuffer::~VulkanCommandBuffer() {
	if (this->m_commandBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(this->m_device, this->m_pool, 1, &this->m_commandBuffer);
		this->m_commandBuffer = VK_NULL_HANDLE;
	}
}

/**
* Begins a command buffer
*/
void 
VulkanCommandBuffer::Begin() {
	VkCommandBufferBeginInfo beginInfo = { };
}

void
VulkanCommandBuffer::End() {
	
}

void
VulkanCommandBuffer::Reset() {
	
}