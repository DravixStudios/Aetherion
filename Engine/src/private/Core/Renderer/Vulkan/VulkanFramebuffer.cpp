#include "Core/Renderer/Vulkan/VulkanFramebuffer.h"

VulkanFramebuffer::VulkanFramebuffer(VkDevice device) 
	: m_device(device), m_framebuffer(VK_NULL_HANDLE) {}

VulkanFramebuffer::~VulkanFramebuffer() {
	if (this->m_framebuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(this->m_device, this->m_framebuffer, nullptr);
	}
}

/**
* Creates a Vulkan framebuffer
* 
* @param createInfo Framebuffer create info
*/
void 
VulkanFramebuffer:: Create(const FramebufferCreateInfo& createInfo) {
	Vector<VkImageView> attachments;
	for (const Ref<ImageView>& attachment : createInfo.attachments) {
		Ref<VulkanImageView> vkAttachment = attachment.As<VulkanImageView>();

		attachments.push_back(vkAttachment->GetVkImageView());
	}

	VkFramebufferCreateInfo fbInfo = { };
	fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.attachmentCount = attachments.size();
	fbInfo.pAttachments = attachments.data();
	fbInfo.renderPass = createInfo.renderPass.As<VulkanRenderPass>()->GetVkRenderPass();
	fbInfo.width = createInfo.nWidth;
	fbInfo.height = createInfo.nHeight;
	fbInfo.layers = createInfo.nLayers;
	VK_CHECK(
		vkCreateFramebuffer(
			this->m_device, 
			&fbInfo, 
			nullptr, 
			&this->m_framebuffer
		), 
		"Failed creating framebuffer");
}