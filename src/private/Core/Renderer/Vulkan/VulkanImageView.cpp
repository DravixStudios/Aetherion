#include "Core/Renderer/Vulkan/VulkanImageView.h"

VulkanImageView::VulkanImageView(VkDevice device) : m_device(device), m_imageView(VK_NULL_HANDLE) {}

VulkanImageView::~VulkanImageView() {
	if (this->m_imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(this->m_device, this->m_imageView, nullptr);
	}
}

/**
* Creates a Vulkan image view
*
* @param createInfo Image view create info
*/
void 
VulkanImageView::Create(const ImageViewCreateInfo& createInfo) {
	VkImageSubresourceRange subresourceRange = { };
	subresourceRange.aspectMask = VulkanHelpers::ConvertImageAspect(createInfo.subresourceRange.aspectMask);
	subresourceRange.baseMipLevel = createInfo.subresourceRange.nBaseMipLevel;
	subresourceRange.levelCount = createInfo.subresourceRange.nLevelCount;
	subresourceRange.baseArrayLayer = createInfo.subresourceRange.nBaseArrayLayer;
	subresourceRange.layerCount = createInfo.subresourceRange.nLayerCount;

	VkComponentMapping components = { };
	components.r = this->ConvertSwizzle(createInfo.components.r);
	components.g = this->ConvertSwizzle(createInfo.components.g);
	components.b = this->ConvertSwizzle(createInfo.components.b);
	components.a = this->ConvertSwizzle(createInfo.components.a);
	
	VkImageViewCreateInfo viewInfo = { };
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.viewType = VulkanHelpers::ConvertImageViewType(createInfo.viewType);
	viewInfo.format = VulkanHelpers::ConvertFormat(createInfo.format);
	viewInfo.subresourceRange = subresourceRange;
	viewInfo.components = components;
	viewInfo.image = createInfo.image.As<VulkanTexture>()->GetVkImage();
	
	VK_CHECK(vkCreateImageView(this->m_device, &viewInfo, nullptr, &this->m_imageView), "Failed creating image view");

	this->m_image = createInfo.image;
	this->m_viewType = createInfo.viewType;
	this->m_format = createInfo.format;
}

/**
* Creates a VulkanImageView from VkImageView
* Note: This overload is exclusive for
* swapchain image views.
*
* @param imageView Vulkan image view
*/
void 
VulkanImageView::Create(const VkImageView& imageView, GPUFormat format) {
	this->m_imageView = imageView;
	this->m_format = format;
}

void 
VulkanImageView::Reset() {
	if (!this->m_device) {
		return;
	}

	if (this->m_imageView != VK_NULL_HANDLE) {
		vkDestroyImageView(this->m_device, this->m_imageView, nullptr);
		this->m_imageView = VK_NULL_HANDLE;
	}

	this->m_image->Reset();
	this->m_viewType = EImageViewType::TYPE_2D;
	this->m_format = GPUFormat::UNDEFINED;
}

/**
* (ESwizzle -> VkComponentSwizzle)
* 
* @param swizzle Swizzle
* 
* @returns Vulkan Component swizzle
*/
VkComponentSwizzle 
VulkanImageView::ConvertSwizzle(ComponentMapping::ESwizzle swizzle) {
	switch (swizzle) {
		case ComponentMapping::ESwizzle::IDENTITY: return VK_COMPONENT_SWIZZLE_IDENTITY;
		case ComponentMapping::ESwizzle::ZERO: return VK_COMPONENT_SWIZZLE_ZERO;
		case ComponentMapping::ESwizzle::ONE: return VK_COMPONENT_SWIZZLE_ONE;
		case ComponentMapping::ESwizzle::R: return VK_COMPONENT_SWIZZLE_R;
		case ComponentMapping::ESwizzle::G: return VK_COMPONENT_SWIZZLE_G;
		case ComponentMapping::ESwizzle::B: return VK_COMPONENT_SWIZZLE_B;
		case ComponentMapping::ESwizzle::A: return VK_COMPONENT_SWIZZLE_A;
		default: return VK_COMPONENT_SWIZZLE_IDENTITY;
	}
}