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
	Ref<VulkanDescriptorSetLayout> vkLayout = layout.As<VulkanDescriptorSetLayout>();

	this->m_pool = vkDescriptorPool->GetVkPool();
	VkDescriptorSetLayout vkLayoutHandle = vkLayout->GetVkLayout();

	/* Descriptor set allocation */
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->m_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &vkLayoutHandle;

	VK_CHECK(
		vkAllocateDescriptorSets(this->m_device, &allocInfo, &this->m_descriptorSet), 
		"Failed allocating descriptor sets"
	);
}

void 
VulkanDescriptorSet::WriteBuffer(
	uint32_t nBinding,
	uint32_t nArrayElement,
	const DescriptorBufferInfo& bufferInfo
) {
	Ref<VulkanBuffer> vkBuffer = bufferInfo.buffer.As<VulkanBuffer>();

	VkDescriptorBufferInfo buffInfo = { };
	buffInfo.buffer = vkBuffer->GetBuffer();
	buffInfo.offset = bufferInfo.nOffset;
	buffInfo.range = bufferInfo.nRange == 0 ? VK_WHOLE_SIZE : bufferInfo.nRange;

	this->m_bufferInfos.push_back(buffInfo);

	VkWriteDescriptorSet write = { };
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = nBinding;
	write.dstArrayElement = nArrayElement;
	write.dstSet = this->m_descriptorSet;
	write.descriptorCount = 1;
	write.pBufferInfo = &this->m_bufferInfos.back();

	/* (EBufferType -> VkDescriptorType) */
	switch (vkBuffer->GetBufferType()) {
	case EBufferType::CONSTANT_BUFFER:
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		break;
	case EBufferType::STORAGE_BUFFER:
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		break;
	default:
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		break;
	}

	this->m_pendingWrites.push_back(write);
}

void 
VulkanDescriptorSet::WriteTexture(uint32_t nBinding, uint32_t nArrayElement, const DescriptorImageInfo& imageInfo) {

}

void 
VulkanDescriptorSet::UpdateWrites() {

}