#pragma once
#include "Core/Renderer/GPUTexture.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>


class VulkanTexture : public GPUTexture {
public:
	VulkanTexture(
		VkDevice& dev, 
		VkPhysicalDevice& physicalDev, 
		VkImage& buffer, 
		VkDeviceMemory& memory, 
		uint32_t nSize, 
		VkImageView& imageView, 
		VkSampler& sampler,
		ETextureType textureType = ETextureType::TEXTURE
	);

	VkDevice GetDevice();
	VkPhysicalDevice GetPhysicalDevice();
	VkImage GetBuffer();
	VkDeviceMemory GetMemory();
	VkImageView GetImageView();
	VkSampler GetSampler();

	uint32_t GetSize() override;
private:
	VkDevice m_dev;
	VkPhysicalDevice m_physicalDev;
	VkImage m_buffer;
	VkDeviceMemory m_memory;
	VkImageView m_imageView;
	VkSampler m_sampler;

	uint32_t m_nSize;
};