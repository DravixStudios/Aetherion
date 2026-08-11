#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device)  
	: DescriptorSetLayout::DescriptorSetLayout(), m_device(device), m_layout(VK_NULL_HANDLE) {}

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
void 
VulkanDescriptorSetLayout::Create(const DescriptorSetLayoutCreateInfo& createInfo) {
	DescriptorSetLayout::Create(createInfo);

	Vector<VkDescriptorSetLayoutBinding> bindings;
	Vector<VkDescriptorBindingFlags> bindingFlags;

	/* Convert bindings to vulkan bindings */
	for(const DescriptorSetLayoutBinding& binding : createInfo.bindings) {
		VkDescriptorSetLayoutBinding vkBinding = { };
		vkBinding.binding = binding.nBinding;
		vkBinding.descriptorType = VulkanHelpers::ConvertDescriptorType(binding.descriptorType);
		vkBinding.descriptorCount = binding.nDescriptorCount;
		vkBinding.stageFlags = VulkanHelpers::ConvertShaderStage(binding.stageFlags);
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