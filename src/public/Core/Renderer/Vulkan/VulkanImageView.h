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
	void Create(const VkImageView& imageView);

	Ref<GPUTexture> GetImage() const override;
	EImageViewType GetViewType() const override;
	GPUFormat GetFormat() const override;

	VkImageView GetVkImageView() const { return this->m_imageView; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanImageView>(device);
	}

private:
	VkDevice m_device;

	VkImageView m_imageView;

	VkComponentSwizzle ConvertSwizzle(ComponentMapping::ESwizzle swizzle);
};