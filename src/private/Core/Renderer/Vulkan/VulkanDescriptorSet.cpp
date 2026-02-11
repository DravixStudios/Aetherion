#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"

VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device) 
	: DescriptorSet::DescriptorSet(), 
	m_device(device), m_descriptorSet(VK_NULL_HANDLE) {}

VulkanDescriptorSet::~VulkanDescriptorSet() {
	if (this->m_descriptorSet != VK_NULL_HANDLE && this->m_poolRef) {
		VkDescriptorPool vkPool = this->m_poolRef.As<VulkanDescriptorPool>()->GetVkPool();
		vkFreeDescriptorSets(this->m_device, vkPool, 1, &this->m_descriptorSet);
	}
}

/* Allocates our descriptor set */
void
VulkanDescriptorSet::Allocate(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) {
	Ref<VulkanDescriptorPool> vkDescriptorPool = pool.As<VulkanDescriptorPool>();
	Ref<VulkanDescriptorSetLayout> vkLayout = layout.As<VulkanDescriptorSetLayout>();

	this->m_poolRef = pool;
	VkDescriptorPool vkPool = vkDescriptorPool->GetVkPool();
	VkDescriptorSetLayout vkLayoutHandle = vkLayout->GetVkLayout();

	/* Descriptor set allocation */
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vkPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &vkLayoutHandle;

	VK_CHECK(
		vkAllocateDescriptorSets(this->m_device, &allocInfo, &this->m_descriptorSet), 
		"Failed allocating descriptor sets"
	);

	this->m_layout = layout;
}

/* Adds a buffer for writing */
void 
VulkanDescriptorSet::WriteBuffer(
	uint32_t nBinding,
	uint32_t nArrayElement,
	const DescriptorBufferInfo& bufferInfo
) {
	Ref<VulkanBuffer> vkBuffer = bufferInfo.buffer.As<VulkanBuffer>();

	VkDescriptorBufferInfo buffInfo = { };
	buffInfo.buffer = vkBuffer->GetVkBuffer();
	buffInfo.offset = bufferInfo.nOffset;
	buffInfo.range = bufferInfo.nRange == 0 ? VK_WHOLE_SIZE : bufferInfo.nRange;

	/* Create a vector for this write operation */
	Vector<VkDescriptorBufferInfo> bufferVec;
	bufferVec.push_back(buffInfo);
	this->m_bufferInfos.push_back(bufferVec);

	VkWriteDescriptorSet write = { };
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = nBinding;
	write.dstArrayElement = nArrayElement;
	write.dstSet = this->m_descriptorSet;
	write.descriptorCount = 1;
	write.pBufferInfo = this->m_bufferInfos.back().data();

	/* (EBufferType -> VkDescriptorType) */
	switch (vkBuffer->GetBufferType()) {
	case EBufferType::UNIFORM_BUFFER:
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

/* Adds a texture for writing */
void 
VulkanDescriptorSet::WriteTexture(
	uint32_t nBinding, 
	uint32_t nArrayElement, 
	const DescriptorImageInfo& imageInfo
) {
	Ref<VulkanTexture> vkTexture = imageInfo.texture.As<VulkanTexture>();

	VkDescriptorImageInfo imgInfo = { };
	imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgInfo.imageView = imageInfo.imageView.As<VulkanImageView>()->GetVkImageView();
	imgInfo.sampler = imageInfo.sampler.As<VulkanSampler>()->GetVkSampler();

	/* Create a vector for this write operation */
	Vector<VkDescriptorImageInfo> imageVec;
	imageVec.push_back(imgInfo);
	this->m_imageInfos.push_back(imageVec);

	VkWriteDescriptorSet write = { };
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.dstSet = this->m_descriptorSet;
	write.dstBinding = nBinding;
	write.dstArrayElement = nArrayElement;
	write.descriptorCount = 1;
	write.pImageInfo = this->m_imageInfos.back().data();

	this->m_pendingWrites.push_back(write);
}

/* 
	Adds many buffers for writing 

	Notes:
		- The buffer will be binded at the specified binding
		and it will bind all the buffers on an array starting
		from nFirstArrayElement to the last buffer.

		E.g: nFirstArrayElement: 0. Buffer count: 5. [0, 4]
*/
void 
VulkanDescriptorSet::WriteBuffers(
	uint32_t nBinding, 
	uint32_t nFirstArrayElement, 
	const Vector<DescriptorBufferInfo>& bufferInfos, 
	EBufferType bufferType
) {
	Vector<VkDescriptorBufferInfo> bufferVec;
	bufferVec.reserve(bufferInfos.size());

	for (const DescriptorBufferInfo& bufferInfo : bufferInfos) {
		Ref<VulkanBuffer> vkBuffer = bufferInfo.buffer.As<VulkanBuffer>();

		VkDescriptorBufferInfo buffInfo = { };
		buffInfo.buffer = vkBuffer->GetVkBuffer();
		buffInfo.offset = bufferInfo.nOffset;
		buffInfo.range = bufferInfo.nRange == 0 ? VK_WHOLE_SIZE : bufferInfo.nRange;

		bufferVec.push_back(buffInfo);
	}
	this->m_bufferInfos.push_back(bufferVec);

	VkWriteDescriptorSet write = { };
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = this->m_descriptorSet;
	write.dstBinding = nBinding;
	write.dstArrayElement = nFirstArrayElement;
	write.descriptorCount = bufferInfos.size();
	write.pBufferInfo = this->m_bufferInfos.back().data();

	/* (EBufferType -> VkDescriptorType) */
	switch (bufferType) {
	case EBufferType::UNIFORM_BUFFER:
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

/*
	Adds many images for writing

	Notes: Same notes as in VulkanDescriptorSet::WriteBuffers
*/
void 
VulkanDescriptorSet::WriteTextures(
	uint32_t nBinding, 
	uint32_t nFirstArrayElement, 
	const Vector<DescriptorImageInfo>& imageInfos
) {
	Vector<VkDescriptorImageInfo> imageVec;
	imageVec.reserve(imageInfos.size());

	for (const DescriptorImageInfo& imageInfo : imageInfos) {
		Ref<VulkanTexture> vkTexture = imageInfo.texture.As<VulkanTexture>();

		VkDescriptorImageInfo imgInfo = { };
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imgInfo.imageView = imageInfo.imageView.As<VulkanImageView>()->GetVkImageView();
		imgInfo.sampler = imageInfo.sampler.As<VulkanSampler>()->GetVkSampler();

		imageVec.push_back(imgInfo);
	}
	this->m_imageInfos.push_back(imageVec);

	VkWriteDescriptorSet write = { };
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = this->m_descriptorSet;
	write.dstBinding = nBinding;
	write.dstArrayElement = nFirstArrayElement;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = imageInfos.size();
	write.pImageInfo = this->m_imageInfos.back().data();

	this->m_pendingWrites.push_back(write);
}

/*
	Writes all the pending descriptor writes
	and clears these vectors:
		- m_pendingWrites
		- m_bufferInfos
		- m_imageInfos
*/
void 
VulkanDescriptorSet::UpdateWrites() {
	if (!this->m_pendingWrites.empty()) {
		vkUpdateDescriptorSets(this->m_device, this->m_pendingWrites.size(), this->m_pendingWrites.data(), 0, nullptr);

		this->m_pendingWrites.clear();
		this->m_bufferInfos.clear();
		this->m_imageInfos.clear();
	}
}