#pragma once
#include "Core/Renderer/PipelineLayout.h"

#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanPipelineLayout : public PipelineLayout {
public:
	using Ptr = Ref<VulkanPipelineLayout>;

	explicit VulkanPipelineLayout(VkDevice device);
	~VulkanPipelineLayout() override;

	void Create(const PipelineLayoutCreateInfo& createInfo) override;
	void Create(VkPipelineLayout layout) { this->m_layout = layout; }

	VkPipelineLayout GetVkLayout() const { return this->m_layout; }

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanPipelineLayout>(device);
	}
private:
	VkDevice m_device;
	VkPipelineLayout m_layout;
};