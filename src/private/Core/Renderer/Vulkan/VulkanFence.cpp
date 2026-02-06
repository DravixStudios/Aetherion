#include "Core/Renderer/Vulkan/VulkanFence.h"

VkFenceCreateFlags
ConvertFlags(EFenceFlags flags) {
	VkFenceCreateFlags vkFlags = 0;

	if (flags == EFenceFlags::SIGNALED) {
		vkFlags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	return vkFlags;
}

VulkanFence::VulkanFence(Ref<VulkanDevice> device) 
	: m_device(device), m_fence(VK_NULL_HANDLE) { }

VulkanFence::~VulkanFence() {
	VkDevice vkDevice = this->m_device.As<VulkanDevice>()->GetVkDevice();

	if (this->m_fence != VK_NULL_HANDLE) {
		vkDestroyFence(vkDevice, this->m_fence, nullptr);
	}
}

/**
* Creates a Vulkan fence
* 
* @param createInfo Fence create info
*/
void
VulkanFence::Create(const FenceCreateInfo& createInfo) {
	VkDevice vkDevice = this->m_device.As<VulkanDevice>()->GetVkDevice();

	VkFenceCreateInfo fenceInfo = { };
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = ConvertFlags(createInfo.flags);
	
	VK_CHECK(
		vkCreateFence(
			vkDevice, 
			&fenceInfo,
			nullptr,
			&this->m_fence
		),
		"Failed to create fence");
}