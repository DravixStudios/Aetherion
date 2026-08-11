#pragma once
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDescriptorSetLayout : public DescriptorSetLayout {
public:
	using Ptr = Ref<VulkanDescriptorSetLayout>;

	explicit VulkanDescriptorSetLayout(VkDevice device);
	~VulkanDescriptorSetLayout() override;

	void Create(const DescriptorSetLayoutCreateInfo& createInfo) override;

	VkDescriptorSetLayout GetVkLayout() const { return this->m_layout; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanDescriptorSetLayout>(device);
	}
private:
	VkDevice m_device;
	VkDescriptorSetLayout m_layout;
};