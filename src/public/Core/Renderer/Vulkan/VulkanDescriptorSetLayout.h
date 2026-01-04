#pragma once
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Utils.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDescriptorSetLayout : public DescriptorSetLayout {
public:
	VulkanDescriptorSetLayout(VkDevice);
	~VulkanDescriptorSetLayout() override;

	void Create(const DescriptorSetLayoutCreateInfo& createInfo) override;

	VkDescriptorSetLayout GetVkLayout() const { return this->m_layout; }
private:
	VkDevice m_device;
	VkDescriptorSetLayout m_layout;

	VkDescriptorType ConvertDescriptorType(EDescriptorType type);
	VkShaderStageFlags ConvertShaderStage(EShaderStage stage);
};