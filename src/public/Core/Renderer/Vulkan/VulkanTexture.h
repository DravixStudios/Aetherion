#pragma once
#include "Core/Renderer/GPUTexture.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

class Device;
class VulkanDevice;

class VulkanTexture : public GPUTexture {
public:
	using Ptr = Ref<VulkanTexture>;

	explicit VulkanTexture(Ref<VulkanDevice> device);
	~VulkanTexture() override;

	void Create(const TextureCreateInfo& createInfo) override;
	void Create(const VkImage& image);

	VkImage GetVkImage() const { return this->m_image; }

	uint32_t GetSize() const { return this->m_nSize; };

	void Reset() override;

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanTexture>(device);
	}
private:
	Ref<VulkanDevice> m_device;
	VkImage m_image;
	VkDeviceMemory m_memory;

	uint32_t m_nSize;

	VkImageType ConvertTextureDimensions(ETextureDimensions dimensions);
	VkImageTiling ConvertTextureTiling(ETextureTiling tiling);
	VkImageUsageFlags ConvertTextureUsage(ETextureUsage usage);
	VkSampleCountFlagBits ConvertSampleCount(ESampleCount samples);
	VkSharingMode ConvertSharingMode(ESharingMode sharingMode);
	VkImageCreateFlags ConvertTextureFlags(ETextureFlags flags);
	VkImageLayout ConvertTextureLayout(ETextureLayout layout);
};