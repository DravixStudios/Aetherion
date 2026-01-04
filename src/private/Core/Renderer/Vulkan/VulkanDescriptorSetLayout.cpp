#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include <spdlog/spdlog.h>

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device) 
	: m_device(device), m_layout(VK_NULL_HANDLE) {}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
	if (this->m_layout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(this->m_device, this->m_layout, nullptr);
	}
}

/* 
	Creation of our VulkanDescriptorSetLayout

	Notes:
		- When a binding has bUpdateAfterBind set
		to true, it will add 2 flags.

			~ VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
			~ VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
		
		- When descriptor set layout create info has
		bUpdateAfterBind enabled, we'll create a 
		descriptor set layout binding flags create info
		and bind our bindingFlags to it (bindless textures' ones)

		It also adds a flag to the layout create info
		that specifies that the pool needs to be
		created with the VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT
		flag.
*/
void VulkanDescriptorSetLayout::Create(const DescriptorSetLayoutCreateInfo& createInfo) {
	DescriptorSetLayout::Create(createInfo);

	Vector<VkDescriptorSetLayoutBinding> bindings;
	Vector<VkDescriptorBindingFlags> bindingFlags;

	/* Convert bindings to vulkan bindings */
	for(const DescriptorSetLayoutBinding& binding : createInfo.bindings) {
		VkDescriptorSetLayoutBinding vkBinding = { };
		vkBinding.binding = binding.nBinding;
		vkBinding.descriptorType = this->ConvertDescriptorType(binding.descriptorType);
		vkBinding.descriptorCount = binding.nDescriptorCount;
		vkBinding.stageFlags = this->ConvertShaderStage(binding.stageFlags);
		vkBinding.pImmutableSamplers = nullptr;

		bindings.push_back(vkBinding);

		/* Bindless flags */
		VkDescriptorBindingFlags flags = 0;
		if (binding.bUpdateAfterBind) {
			flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
		}
		bindingFlags.push_back(flags);
	}

	/* Descriptor set layout create info */
	VkDescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	/* If supports update after bind */
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = { };
	if (createInfo.bUpdateAfterBind) {
		/* Initialize layout binding flags create info */
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = bindingFlags.size(); // Get bindingFlags size
		bindingFlagsInfo.pBindingFlags = bindingFlags.data();

		/* 
			Concat create infos and add VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT flag.
			VkDescriptorSetLayoutCreateFlagBits reference:
				https://docs.vulkan.org/refpages/latest/refpages/source/VkDescriptorSetLayoutCreateFlagBits.html
			
		*/
		layoutInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutInfo.pNext = &bindingFlagsInfo;
	}

	VK_CHECK(vkCreateDescriptorSetLayout(this->m_device, &layoutInfo, nullptr, &this->m_layout), "(Vulkan) Failed creating descriptor set layout.");
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