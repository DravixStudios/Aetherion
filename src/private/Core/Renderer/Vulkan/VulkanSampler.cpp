#include "Core/Renderer/Vulkan/VulkanSampler.h"

VulkanSampler::VulkanSampler(Ref<VulkanDevice> device) 
	: m_device(device), m_sampler(VK_NULL_HANDLE) { }

VulkanSampler::~VulkanSampler() {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	if (this->m_sampler != VK_NULL_HANDLE) {
		vkDestroySampler(vkDevice, this->m_sampler, nullptr);
	}
}


/**
* Creates a Vulkan sampler
*
* @param createInfo Sampler create info
*/
void
VulkanSampler::Create(const SamplerCreateInfo& createInfo) {
	VkSamplerCreateInfo samplerInfo = { };
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.flags = this->ConvertFlags(createInfo.flags);
	samplerInfo.magFilter = this->ConvertFilter(createInfo.magFilter);
	samplerInfo.minFilter = this->ConvertFilter(createInfo.minFilter);
	samplerInfo.mipmapMode = this->ConvertMipmapMode(createInfo.mipmapMode);
	
	samplerInfo.addressModeU = this->ConvertAddressMode(createInfo.addressModeU);
	samplerInfo.addressModeV = this->ConvertAddressMode(createInfo.addressModeV);
	samplerInfo.addressModeW = this->ConvertAddressMode(createInfo.addressModeW);

	samplerInfo.mipLodBias = createInfo.mipLodBias;
	samplerInfo.anisotropyEnable = createInfo.bAnisotropyEnable ? VK_TRUE : VK_FALSE;
	samplerInfo.maxAnisotropy = createInfo.maxAnisotropy;

	samplerInfo.compareEnable = createInfo.bCompareEnable ? VK_TRUE : VK_FALSE;
	samplerInfo.compareOp = VulkanHelpers::ConvertCompareOp(createInfo.compareOp);

	samplerInfo.minLod = createInfo.minLod;
	samplerInfo.maxLod = createInfo.maxLod;

	samplerInfo.borderColor = this->ConvertBorderColor(createInfo.borderColor);
	samplerInfo.unnormalizedCoordinates = createInfo.bUnnormalizedCoordinates ? VK_TRUE : VK_FALSE;

	VK_CHECK(
		vkCreateSampler(
			this->m_device->GetVkDevice(),
			&samplerInfo, 
			nullptr, 
			&this->m_sampler
		),
		"Failed creating sampler");
}

/**
* (ESamplerFlags -> VkSamplerCreateFlagBits)
* 
* @param flags Flags
* 
* @returns Vulkan sampler create flags
*/
VkSamplerCreateFlags
VulkanSampler::ConvertFlags(ESamplerFlags flags) {
	VkSamplerCreateFlags vkFlags = 0;

	if ((flags & ESamplerFlags::SUBSAMPLED) != static_cast<ESamplerFlags>(0)) {
		vkFlags |= VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT;
	}

	if ((flags & ESamplerFlags::SUBSAMPLED_COARSE_RECONSTRUCTION) != static_cast<ESamplerFlags>(0)) {
		vkFlags |= VK_SAMPLER_CREATE_SUBSAMPLED_COARSE_RECONSTRUCTION_BIT_EXT;
	}

	if ((flags & ESamplerFlags::DESCRIPTOR_BUFFER_CAPTURE_REPLAY) != static_cast<ESamplerFlags>(0)) {
		vkFlags |= VK_SAMPLER_CREATE_DESCRIPTOR_BUFFER_CAPTURE_REPLAY_BIT_EXT;
	}

	if ((flags & ESamplerFlags::NON_SEAMLESS_CUBE_MAP) != static_cast<ESamplerFlags>(0)) {
		vkFlags |= VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT;
	}

	if ((flags & ESamplerFlags::IMAGE_PROCESSING_QCOM) != static_cast<ESamplerFlags>(0)) {
		vkFlags |= VK_SAMPLER_CREATE_IMAGE_PROCESSING_BIT_QCOM;
	}

	return vkFlags;
}

/**
* (EFilter -> VkFilter)
* 
* @param filter Filter
* 
* @returns Vulkan filter
*/
VkFilter 
VulkanSampler::VulkanSampler::ConvertFilter(EFilter filter) {
	switch (filter) {
		case EFilter::NEAREST: return VK_FILTER_NEAREST;
		case EFilter::LINEAR: return VK_FILTER_LINEAR;
		case EFilter::CUBIC: return VK_FILTER_CUBIC_EXT;
		case EFilter::CUBIC_IMG: return VK_FILTER_CUBIC_IMG;
		default: return VK_FILTER_LINEAR;
	}
}

/**
* (EMipmapMode -> ESamplerMipmapMode)
* 
* @param mode Mipmap mode
* 
* @returns Vulkan sampler mipmap mode
*/
VkSamplerMipmapMode 
VulkanSampler::ConvertMipmapMode(EMipmapMode mode) {
	switch (mode) {
		case EMipmapMode::MIPMAP_MODE_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
		case EMipmapMode::MIPMAP_MODE_LINEAR: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}

/**
* (EAddressMode -> VkSamplerAddressMode)
* 
* @param mode Address mode
* 
* @returns Vulkan sampler address mode
*/
VkSamplerAddressMode 
VulkanSampler::ConvertAddressMode(EAddressMode mode) {
	switch (mode) {
		case EAddressMode::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case EAddressMode::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case EAddressMode::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case EAddressMode::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case EAddressMode::MIRROR_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

/**
* (EBorderColor -> VkBorderColor)
* 
* @param borderColor Border color
* 
* @returns Vulkan border color
*/
VkBorderColor 
VulkanSampler::ConvertBorderColor(EBorderColor borderColor) {
	switch (borderColor) {
		case EBorderColor::FLOAT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case EBorderColor::INT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		case EBorderColor::FLOAT_OPAQUE_BLACK: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case EBorderColor::INT_OPAQUE_BLACK: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		case EBorderColor::FLOAT_OPAQUE_WHITE: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case EBorderColor::INT_OPAQUE_WHITE: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		case EBorderColor::FLOAT_CUSTOM: return VK_BORDER_COLOR_FLOAT_CUSTOM_EXT;
		case EBorderColor::INT_CUSTOM: return VK_BORDER_COLOR_INT_CUSTOM_EXT;
		default: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	}
}