#include "Core/Renderer/Vulkan/VulkanTexture.h"
#include "Core/Renderer/Vulkan/VulkanDevice.h"

VulkanTexture::VulkanTexture(Ref<VulkanDevice> device) 
	: m_device(device), m_image(VK_NULL_HANDLE), m_nSize(0) {}

VulkanTexture::~VulkanTexture() {
	if (this->m_image != VK_NULL_HANDLE) {
		vkDestroyImage(this->m_device->GetVkDevice(), this->m_image, nullptr);
	}
}

/**
* Creates a Vulkan GPU texture
* 
* @param createInfo Texture create info
*/
void 
VulkanTexture::Create(const TextureCreateInfo& createInfo) {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	/* Create image */
	VkExtent3D extent = { };
	extent.width = createInfo.extent.width;
	extent.height = createInfo.extent.height;
	extent.depth = createInfo.extent.depth;

	VkImageCreateInfo imageInfo = { };
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = this->ConvertTextureFlags(createInfo.flags);
	imageInfo.imageType = this->ConvertTextureDimensions(createInfo.imageType);
	imageInfo.format = VulkanHelpers::ConvertFormat(createInfo.format);
	imageInfo.extent = extent;
	imageInfo.mipLevels = createInfo.nMipLevels;
	imageInfo.arrayLayers = createInfo.nArrayLayers;
	imageInfo.samples = this->ConvertSampleCount(createInfo.samples);
	imageInfo.tiling = this->ConvertTextureTiling(createInfo.tiling);
	imageInfo.usage = this->ConvertTextureUsage(createInfo.usage);
	imageInfo.sharingMode = this->ConvertSharingMode(createInfo.sharingMode);
	imageInfo.queueFamilyIndexCount = createInfo.nQueueFamilyIndexCount;
	imageInfo.pQueueFamilyIndices = createInfo.pQueueFamilyIndices;
	imageInfo.initialLayout = this->ConvertTextureLayout(createInfo.initialLayout);

	VK_CHECK(vkCreateImage(vkDevice, &imageInfo, nullptr, &this->m_image), "Failed creating a image");
	
	/* Allocate image memory */
	Ref<VulkanBuffer> buffer = createInfo.buffer.As<VulkanBuffer>();

	VkMemoryRequirements memReqs = { };
	vkGetImageMemoryRequirements(vkDevice, this->m_image, &memReqs);

	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->m_device->FindMemoryType(
		memReqs.memoryTypeBits, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	VK_CHECK(vkAllocateMemory(vkDevice, &allocInfo, nullptr, &this->m_memory), "Failed allocating image memory");

	VK_CHECK(vkBindImageMemory(vkDevice, this->m_image, this->m_memory, 0), "Failed binding image memory");

	/* Copy buffer to image */
	Ref<CommandBuffer> commandBuff = this->m_device->BeginSingleTimeCommandBuffer();

	VkBufferImageCopy region = { };
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { createInfo.extent.width, createInfo.extent.height, 1 };

	Ref<VulkanCommandBuffer> vkCommandBuff = commandBuff.As<VulkanCommandBuffer>();

	vkCmdCopyBufferToImage(
		vkCommandBuff->GetVkCommandBuffer(),
		buffer->GetVkBuffer(), 
		this->m_image, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);

	this->m_device->EndSingleTimeCommandBuffer(commandBuff);
}

/**
* (ETextureDimensions -> VkImageType)
* 
* @param dimensions Texture dimensions
* 
* @returns Vulkan image type
*/
VkImageType 
VulkanTexture::ConvertTextureDimensions(ETextureDimensions dimensions) {
	switch (dimensions) {
		case ETextureDimensions::TYPE_1D: return VK_IMAGE_TYPE_1D;
		case ETextureDimensions::TYPE_2D: return VK_IMAGE_TYPE_2D;
		case ETextureDimensions::TYPE_3D: return VK_IMAGE_TYPE_3D;
		default: return VK_IMAGE_TYPE_2D;
	}
}

/**
* (ETextureTiling -> VkImageTiling)
* 
* @param tiling Texture tiling
* 
* @returns Vulkan image tiling
*/
VkImageTiling 
VulkanTexture::ConvertTextureTiling(ETextureTiling tiling) {
	switch(tiling) {
		case ETextureTiling::OPTIMAL: return VK_IMAGE_TILING_OPTIMAL;
		case ETextureTiling::LINEAR: return VK_IMAGE_TILING_LINEAR;
		default: return VK_IMAGE_TILING_OPTIMAL;
	}
}

/**
* (ETextureUsage -> VkImageUsageFlags)
* 
* @param usage Texture usage
* 
* @returns Vulkan image usage flags
*/
VkImageUsageFlags 
VulkanTexture::ConvertTextureUsage(ETextureUsage usage) {
	VkImageUsageFlags flags = 0;

	if ((usage & ETextureUsage::TRANSFER_SRC) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	if ((usage & ETextureUsage::TRANSFER_DST) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if ((usage & ETextureUsage::SAMPLED) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if ((usage & ETextureUsage::STORAGE) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if ((usage & ETextureUsage::COLOR_ATTACHMENT) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if ((usage & ETextureUsage::DEPTH_STENCIL_ATTACHMENT) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	if ((usage & ETextureUsage::TRANSIENT_ATTACHMENT) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}

	if ((usage & ETextureUsage::INPUT_ATTACHMENT) != static_cast<ETextureUsage>(0)) {
		flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}

	return flags;
}

/**
* (ESampleCount -> VkSampleCountFlags)
* 
* @param samples Sample count
* 
* @returns Vulkan sample count flags
*/
VkSampleCountFlagBits
VulkanTexture::ConvertSampleCount(ESampleCount samples) {
	switch (samples) {
		case ESampleCount::SAMPLE_1: return VK_SAMPLE_COUNT_1_BIT;
		case ESampleCount::SAMPLE_2: return VK_SAMPLE_COUNT_2_BIT;
		case ESampleCount::SAMPLE_4: return VK_SAMPLE_COUNT_4_BIT;
		case ESampleCount::SAMPLE_8: return VK_SAMPLE_COUNT_8_BIT;
		case ESampleCount::SAMPLE_16: return VK_SAMPLE_COUNT_16_BIT;
		case ESampleCount::SAMPLE_32: return VK_SAMPLE_COUNT_32_BIT;
		case ESampleCount::SAMPLE_64: return VK_SAMPLE_COUNT_64_BIT;
		default: return VK_SAMPLE_COUNT_1_BIT;
	}
}

/**
* (ESharingMode -> VkSharingMode)
* 
* @param sharingMode Sharing mode
* 
* @returns Vulkan sharing mode
*/
VkSharingMode 
VulkanTexture::ConvertSharingMode(ESharingMode sharingMode) {
	switch (sharingMode) {
		case ESharingMode::EXCLUSIVE: return VK_SHARING_MODE_EXCLUSIVE;
		case ESharingMode::CONCURRENT: return VK_SHARING_MODE_CONCURRENT;
		default: return VK_SHARING_MODE_EXCLUSIVE;
	}
}

/**
* (ETextureFlags -> VkImageCreateFlags)
* 
* @param flags Texture flags
* 
* @returns Vulkan image create flags
*/
VkImageCreateFlags 
VulkanTexture::ConvertTextureFlags(ETextureFlags flags) {
	VkImageCreateFlags vkFlags = 0;

	if ((flags & ETextureFlags::SPARSE_BINDING) != static_cast<ETextureFlags>(0)) {
		vkFlags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
	}

	if ((flags & ETextureFlags::SPARSE_RESIDENCY) != static_cast<ETextureFlags>(0)) {
		vkFlags |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
	}

	if ((flags & ETextureFlags::SPARSE_ALIASED) != static_cast<ETextureFlags>(0)) {
		vkFlags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;
	}

	if ((flags & ETextureFlags::MUTABLE_FORMAT) != static_cast<ETextureFlags>(0)) {
		vkFlags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	}

	if ((flags & ETextureFlags::CUBE_COMPATIBLE) != static_cast<ETextureFlags>(0)) {
		vkFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	
	return vkFlags;
}

/**
* (ETextureLayout -> VkImageLayout)
* 
* @param layout Texture layout
* 
* @returns Vulkan image layout
*/
VkImageLayout 
VulkanTexture::ConvertTextureLayout(ETextureLayout layout) {
	switch (layout) {
		case ETextureLayout::UNDEFINED: return VK_IMAGE_LAYOUT_UNDEFINED;
		case ETextureLayout::GENERAL: return VK_IMAGE_LAYOUT_GENERAL;
		case ETextureLayout::COLOR_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ETextureLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ETextureLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ETextureLayout::SHADER_READ_ONLY_OPTIMAL: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ETextureLayout::TRANSFER_SRC_OPTIMAL: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ETextureLayout::TRANSFER_DST_OPTIMAL: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ETextureLayout::PREINITIALIZED: return VK_IMAGE_LAYOUT_PREINITIALIZED;
		default: return VK_IMAGE_LAYOUT_UNDEFINED;
	}
}