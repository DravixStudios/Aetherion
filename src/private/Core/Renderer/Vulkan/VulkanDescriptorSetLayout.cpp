#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include <spdlog/spdlog.h>

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device) 
	: m_device(device), m_layout(VK_NULL_HANDLE) {}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
	if (this->m_layout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(this->m_device, this->m_layout, nullptr);
	}
}

void VulkanDescriptorSetLayout::Create(const DescriptorSetLayoutCreateInfo& createInfo) {
	DescriptorSetLayout::Create(createInfo);

	Vector<VkDescriptorSetLayoutBinding> bindings;
	Vector<VkDescriptorBindingFlags> bindingFlags;

	/* Convert bindings to vulkan bindings */
	for(const DescriptorSetLayoutBinding& binding : createInfo.bindings) {
		
	}
}

/* Converts EDescriptorType into VkDescriptorType */
VkDescriptorType 
VulkanDescriptorSetLayout::ConvertDescriptorType(EDescriptorType type) {
	switch (type) {
		case EDescriptorType::SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case EDescriptorType::COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case EDescriptorType::SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case EDescriptorType::STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case EDescriptorType::UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case EDescriptorType::STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case EDescriptorType::UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case EDescriptorType::STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case EDescriptorType::UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case EDescriptorType::STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case EDescriptorType::INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		default: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}
}

/* Converts EShaderStage into VkShaderStageFlags */
VkShaderStageFlags 
VulkanDescriptorSetLayout::ConvertShaderStage(EShaderStage stage) {
	VkShaderStageFlags flags = 0;


	if ((stage & EShaderStage::VERTEX) != static_cast<EShaderStage>(0))
		flags |= VK_SHADER_STAGE_VERTEX_BIT;
	if ((stage & EShaderStage::FRAGMENT) != static_cast<EShaderStage>(0))
		flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if ((stage & EShaderStage::COMPUTE) != static_cast<EShaderStage>(0))
		flags |= VK_SHADER_STAGE_COMPUTE_BIT;
	if ((stage & EShaderStage::TESSELATION_CONTROL) != static_cast<EShaderStage>(0))
		flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
	if ((stage & EShaderStage::TESSELATION_EVALUATION) != static_cast<EShaderStage>(0))
		flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	/* 
		Vulkan doesn't allow to create a descriptor set layout 
		without stages, so we create it with all the possible
		shader stages and done
	*/
	if (flags == 0) flags = VK_SHADER_STAGE_ALL;

	return flags;
}