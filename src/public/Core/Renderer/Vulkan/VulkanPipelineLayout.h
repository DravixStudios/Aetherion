#pragma once
#include "Core/Renderer/PipelineLayout.h"

class VulkanPipelineLayout : public PipelineLayout {
public:
	using Ptr = Ref<VulkanPipelineLayout>;

	explicit VulkanPipelineLayout(VkDevice device);
	~VulkanPipelineLayout() override;

	void Create(const PipelineLayoutCreateInfo& createInfo) override;

	static Ptr
	CreateShared(VkDevice device) {
		return CreateRef<VulkanPipelineLayout>(device);
	}
private:
	VkDevice m_device;
	VkPipelineLayout m_layout;
};