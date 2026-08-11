#pragma once
#include "Core/Renderer/ImageView.h"

#include "Core/Renderer/Vulkan/VulkanHelpers.h"
#include "Core/Renderer/Vulkan/VulkanTexture.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanImageView : public ImageView {
public:
	using Ptr = Ref<VulkanImageView>;

	explicit VulkanImageView(VkDevice device);
	~VulkanImageView() override;

	void Create(const ImageViewCreateInfo& createInfo) override;
	void Create(const VkImageView& imageView, GPUFormat format);

	Ref<GPUTexture> GetImage() const override { return this->m_image; }
	EImageViewType GetViewType() const override { return this->m_viewType; }
	GPUFormat GetFormat() const override { return this->m_format; }

	VkImageView GetVkImageView() const { return this->m_imageView; }

	void Reset() override;

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanImageView>(device);
	}

private:
	VkDevice m_device;

	EImageViewType m_viewType;
	GPUFormat m_format;

	Ref<GPUTexture> m_image;
	VkImageView m_imageView;

	VkComponentSwizzle ConvertSwizzle(ComponentMapping::ESwizzle swizzle);
};