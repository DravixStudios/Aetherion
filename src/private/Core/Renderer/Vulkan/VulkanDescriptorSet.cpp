#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"

VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device) 
	: DescriptorSet::DescriptorSet(), 
	m_device(device), m_descriptorSet(VK_NULL_HANDLE), m_pool(VK_NULL_HANDLE) {}

VulkanDescriptorSet::~VulkanDescriptorSet() {
	if (this->m_descriptorSet != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(this->m_device, this->m_pool, 1, &this->m_descriptorSet);
	}
}

void
VulkanDescriptorSet::Allocate(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) {
	Ref<VulkanDescriptorPool> vkDescriptorPool = pool.As<VulkanDescriptorPool>();


}

void 
VulkanDescriptorSet::WriteBuffer(uint32_t nBinding, uint32_t nArrayElement, const DescriptorBufferInfo& bufferInfo) {

}

void 
VulkanDescriptorSet::WriteTexture(uint32_t nBinding, uint32_t nArrayElement, const DescriptorImageInfo& imageInfo) {

}

void 
VulkanDescriptorSet::UpdateWrites() {

}