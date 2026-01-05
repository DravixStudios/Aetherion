#pragma once
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Utils.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDescriptorSetLayout : public DescriptorSetLayout {
public:
	VulkanDescriptorSetLayout(VkDevice device);
	~VulkanDescriptorSetLayout() override;

	void Create(const DescriptorSetLayoutCreateInfo& createInfo) override;

	VkDescriptorSetLayout GetVkLayout() const { return this->m_layout; }
private:
	VkDevice m_device;
	VkDescriptorSetLayout m_layout;
};