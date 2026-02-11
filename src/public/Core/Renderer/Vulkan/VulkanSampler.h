#pragma once
#include "Core/Renderer/Sampler.h"

#include "Core/Renderer/Vulkan/VulkanDevice.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanSampler : public Sampler {
public:
	using Ptr = Ref<VulkanSampler>;

	explicit VulkanSampler(Ref<VulkanDevice> device);
	~VulkanSampler() override;

	void Create(const SamplerCreateInfo& createInfo) override;

	VkSampler GetVkSampler() const { return this->m_sampler; }

	static Ptr
	CreateShared(Ref<VulkanDevice> device) {
		return CreateRef<VulkanSampler>(device);
	}

private:
	Ref<VulkanDevice> m_device;

	VkSampler m_sampler;

	VkSamplerCreateFlags ConvertFlags(ESamplerFlags flags);
	VkFilter ConvertFilter(EFilter filter);
	VkSamplerMipmapMode ConvertMipmapMode(EMipmapMode mode);
	VkSamplerAddressMode ConvertAddressMode(EAddressMode mode);
	VkBorderColor ConvertBorderColor(EBorderColor borderColor);
};