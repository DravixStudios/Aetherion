#include "Core/Renderer/Vulkan/VulkanCommandPool.h"

VulkanCommandPool::VulkanCommandPool(VkDevice device) 
	: m_device(device), m_pool(VK_NULL_HANDLE) {}

VulkanCommandPool::~VulkanCommandPool() {
	if (this->m_pool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(this->m_device, this->m_pool, nullptr);
	}
}

/**
* Creates a Vulkan command pool
* 
* @param createInfo Command pool create info
*/
void 
VulkanCommandPool::Create(const CommandPoolCreateInfo& createInfo) {
	VkCommandPoolCreateInfo poolInfo = { };
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = createInfo.nQueueFamilyIndex;
	poolInfo.flags = this->ConvertCommandPoolFlags(createInfo.flags);
	
	VK_CHECK(vkCreateCommandPool(this->m_device, &poolInfo, nullptr, &this->m_pool), "Failed creating command pool");
}

/**
* Allocates a Vulkan command buffer from the current pool
* 
* @return A command buffer
*/
Ref<CommandBuffer> 
VulkanCommandPool::AllocateCommandBuffer() {
	VkCommandBuffer vkCommandBuff = nullptr;

	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->m_pool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VK_CHECK(vkAllocateCommandBuffers(this->m_device, &allocInfo, &vkCommandBuff), "Failed allocating command buffer");

	Ref<VulkanCommandBuffer> commandBuff = VulkanCommandBuffer::CreateShared(this->m_device, vkCommandBuff);
	return commandBuff.As<CommandBuffer>();
}

/**
* Allocates many Vulkan command buffers from the current pool
* 
* @param nCount Command buffer count
* 
* @returns A vector of Vulkan command buffers
*/
Vector<Ref<CommandBuffer>> 
VulkanCommandPool::AllocateCommandBuffers(uint32_t nCount) {
	Vector<VkCommandBuffer> vkCommandBuffers;
	vkCommandBuffers.resize(nCount);

	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->m_pool;
	allocInfo.commandBufferCount = nCount;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VK_CHECK(
		vkAllocateCommandBuffers(
			this->m_device, 
			&allocInfo, 
			vkCommandBuffers.data()
		), "Failed allocating command buffers");

	Vector<Ref<CommandBuffer>> commandBuffers;
	commandBuffers.resize(nCount);

	for (uint32_t i = 0; i < nCount; i++) {
		VkCommandBuffer vkCommandBuffer = vkCommandBuffers[i];

		Ref<VulkanCommandBuffer> commandBuffer = VulkanCommandBuffer::CreateShared(this->m_device, vkCommandBuffer);

		commandBuffers[i] = commandBuffer.As<CommandBuffer>();
	}
	
	return commandBuffers;
}

/**
* Frees a command buffer
* 
* @param commandBuffer Command buffer to free
*/
void 
VulkanCommandPool::FreeCommandBuffer(Ref<CommandBuffer> commandBuffer) {
	VkCommandBuffer vkCommandBuffer = commandBuffer.As<VulkanCommandBuffer>()->GetVkCommandBuffer();

	vkFreeCommandBuffers(this->m_device, this->m_pool, 1, &vkCommandBuffer);
}

void 
VulkanCommandPool::FreeCommandBuffers(const Vector<Ref<CommandBuffer>>& commandBuffers) {
	uint32_t nCommandBuffCount = commandBuffers.size();

	Vector<VkCommandBuffer> vkCommandBuffers;
	vkCommandBuffers.resize(nCommandBuffCount);

	for (uint32_t i = 0; i < nCommandBuffCount; i++) {
		VkCommandBuffer vkCommandBuffer = commandBuffers[i].As<VulkanCommandBuffer>()->GetVkCommandBuffer();
		vkCommandBuffers[i] = vkCommandBuffer;
	}

	vkFreeCommandBuffers(this->m_device, this->m_pool, nCommandBuffCount, vkCommandBuffers.data());
}

/**
* Resets the Vulkan command pool freeing all the command buffers
* 
* @param bReleaseResources If true, release the resources
*/
void 
VulkanCommandPool::Reset(bool bReleaseResources) {
	VkCommandPoolResetFlags resetFlags = 0;

	if (bReleaseResources) {
		resetFlags |= VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT;
	}

	VK_CHECK(vkResetCommandPool(this->m_device, this->m_pool, resetFlags), "Failed to reset a command pool");
}

/**
* (ECommandPoolFlags -> VkCommandPoolCreateFlags)
* 
* @param flags Command pool flags (ECommandPoolFlags)
*/
VkCommandPoolCreateFlags 
VulkanCommandPool::ConvertCommandPoolFlags(ECommandPoolFlags flags) {
	VkCommandPoolCreateFlags vkFlags = 0;
	if ((flags & ECommandPoolFlags::TRANSIENT) != ECommandPoolFlags::NONE) {
		vkFlags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	}

	if ((flags & ECommandPoolFlags::RESET_COMMAND_BUFFER) != ECommandPoolFlags::NONE) {
		vkFlags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	}

	return vkFlags;
}