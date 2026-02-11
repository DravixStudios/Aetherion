#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"
#include "Core/Renderer/Vulkan/VulkanDevice.h"

VulkanCommandBuffer::VulkanCommandBuffer(Ref<VulkanDevice> device, VkCommandBuffer buffer)
	: m_device(device), m_commandBuffer(buffer) { }

VulkanCommandBuffer::~VulkanCommandBuffer() {
	if (this->m_commandBuffer != VK_NULL_HANDLE) {
		this->m_commandBuffer = VK_NULL_HANDLE;
	}
}

void 
VulkanCommandBuffer::Begin(bool bSingleTime) {
	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = bSingleTime ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;

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