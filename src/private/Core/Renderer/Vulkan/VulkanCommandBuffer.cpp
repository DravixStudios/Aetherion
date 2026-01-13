#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool)
	: m_device(device), m_commandBuffer(VK_NULL_HANDLE), m_pool(pool) { }

VulkanCommandBuffer::~VulkanCommandBuffer() {
	if (this->m_commandBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(this->m_device, this->m_pool, 1, &this->m_commandBuffer);
		this->m_commandBuffer = VK_NULL_HANDLE;
	}
}

void 
VulkanCommandBuffer::Begin() {
	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(this->m_commandBuffer, &beginInfo), "Failed to begin a command buffer");
	
}

void
VulkanCommandBuffer::End() {
	VK_CHECK(vkEndCommandBuffer(this->m_commandBuffer), "Failed to end a command buffer");
}

void
VulkanCommandBuffer::Reset() {
	VK_CHECK(vkResetCommandBuffer(this->m_commandBuffer, 0), "Failed to reset a command buffer");
}