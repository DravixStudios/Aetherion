#include "Core/Renderer/Vulkan/VulkanDescriptorPool.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device) 
	: DescriptorPool::DescriptorPool(), m_device(device), m_pool(VK_NULL_HANDLE) {}

VulkanDescriptorPool::~VulkanDescriptorPool() {
	if (this->m_pool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(this->m_device, this->m_pool, nullptr);
	}
}

void 
VulkanDescriptorPool::Create(const DescriptorPoolCreateInfo& createInfo) {
	/* Convert DescriptorPoolSize to Vulkan descriptor pool size */
	Vector<VkDescriptorPoolSize> vkPoolSizes;

	for (DescriptorPoolSize poolSize : createInfo.poolSizes) {
		VkDescriptorPoolSize vkSize = { };
		vkSize.type = VulkanHelpers::ConvertDescriptorType(poolSize.type);
		vkSize.descriptorCount = poolSize.nDescriptorCount;
		
		vkPoolSizes.push_back(vkSize);
	}

	/* Creation of the descriptor pool */
	VkDescriptorPoolCreateInfo poolInfo = { };
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = vkPoolSizes.size();
	poolInfo.pPoolSizes = vkPoolSizes.data();
	poolInfo.maxSets = createInfo.nMaxSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (createInfo.bUpdateAfterBind) {
		poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
	}

	VK_CHECK(vkCreateDescriptorPool(this->m_device, &poolInfo, nullptr, &this->m_pool), "Failed creating descriptor pool");

	Logger::Debug("VulkanDescriptorPool::Create: Descriptor pool created. Max sets: {}", createInfo.nMaxSets);
}

void 
VulkanDescriptorPool::Reset() {
	vkResetDescriptorPool(this->m_device, this->m_pool, 0);
}