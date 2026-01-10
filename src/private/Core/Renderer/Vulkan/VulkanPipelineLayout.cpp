#include "Core/Renderer/Vulkan/VulkanPipelineLayout.h"

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device) 
	: m_device(device), m_layout(VK_NULL_HANDLE) {}

VulkanPipelineLayout::~VulkanPipelineLayout() {
	if (this->m_layout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(this->m_device, this->m_layout, nullptr);
	}
}

/**
* Creates a Vulkan pipeline layout
* 
* @param createInfo Pipeline layout create info
*/
void 
VulkanPipelineLayout::Create(const PipelineLayoutCreateInfo& createInfo) {
	/* (DescriptorSetLayout -> VkDescriptorSetLayout) */
	Vector<VkDescriptorSetLayout> setLayouts;
	for (const Ref<DescriptorSetLayout>& setLayout : createInfo.setLayouts) {
		Ref<VulkanDescriptorSetLayout> vkLayout = setLayout.As<VulkanDescriptorSetLayout>();

		setLayouts.push_back(vkLayout->GetVkLayout());
	}

	/* (PushConstantRange -> VkPushConstantRange) */
	Vector<VkPushConstantRange> pushConstantRanges;
	for (const PushConstantRange range : createInfo.pushConstantRanges) {
		VkPushConstantRange vkRange = { };
		vkRange.offset = range.nOffset;
		vkRange.size = range.nSize;
		vkRange.stageFlags = VulkanHelpers::ConvertShaderStage(range.stage);
	
		pushConstantRanges.push_back(vkRange);
	}

	/* Pipeline layout create info */
	VkPipelineLayoutCreateInfo plInfo = { };
	plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plInfo.setLayoutCount = setLayouts.size();
	plInfo.pSetLayouts = setLayouts.data();
	plInfo.pushConstantRangeCount = pushConstantRanges.size();
	plInfo.pPushConstantRanges = pushConstantRanges.data();

	VK_CHECK(
		vkCreatePipelineLayout(
			this->m_device, 
			&plInfo, 
			nullptr, 
			&this->m_layout
		), 
		"Failed creating pipeline layout");
}