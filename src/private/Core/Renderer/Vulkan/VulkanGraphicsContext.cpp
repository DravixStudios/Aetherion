#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"

VulkanGraphicsContext::VulkanGraphicsContext(VkCommandBuffer commandBuffer) 
	: m_commandBuffer(commandBuffer), m_currentPipeline(VK_NULL_HANDLE), 
	m_currentBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM), m_currentPipelineLayout(VK_NULL_HANDLE) {}

/**
* Binds a Vulkan pipeline
* 
* @param pipeline Binded pipeline
*/
void 
VulkanGraphicsContext::BindPipeline(Ref<Pipeline> pipeline) {
	Ref<VulkanPipeline> vkPipeline = pipeline.As<VulkanPipeline>();
	this->m_currentPipeline = vkPipeline->GetVkPipeline();
	this->m_currentBindPoint = vkPipeline->GetVkBindPoint();

	vkCmdBindPipeline(this->m_commandBuffer, this->m_currentBindPoint, this->m_currentPipeline);
}

/**
* Binds descriptor sets
* 
* @param nFirstSet
* @param sets DescriptorSet vector
* @param dynamicOffsets  Dynamic offsets (optional)
*/
void 
VulkanGraphicsContext::BindDescriptorSets(
	uint32_t nFirstSet,
	const Vector<Ref<DescriptorSet>>& sets, 
	const Vector<uint32_t>& dynamicOffsets
) {
	uint32_t nSetCount = sets.size();

	Vector<VkDescriptorSet> vkDescriptorSets;
	vkDescriptorSets.resize(nSetCount);

	for (uint32_t i = 0; i < nSetCount; i++) {
		const Ref<DescriptorSet>& descriptorSet = sets[i];
		vkDescriptorSets[i] = descriptorSet.As<VulkanDescriptorSet>()->GetVkSet();
	}

	vkCmdBindDescriptorSets(
		this->m_commandBuffer, 
		this->m_currentBindPoint, 
		this->m_currentPipelineLayout, 
		nFirstSet, 
		nSetCount, 
		vkDescriptorSets.data(), 
		dynamicOffsets.size(), dynamicOffsets.data()
	);
}

/**
* Binds vertex buffers
* 
* @param buffers Vertex buffer vector
* @param offsets Offsets (default = {})
*/
void 
VulkanGraphicsContext::BindVertexBuffers(
	const Vector<Ref<GPUBuffer>>& buffers, 
	const Vector<size_t>& offsets
) {
	uint32_t nBufferCount = buffers.size();

	Vector<VkBuffer> vkBuffers;
	vkBuffers.resize(nBufferCount);

	for (uint32_t i = 0; i < nBufferCount; i++) {
		const Ref<GPUBuffer>& buffer = buffers[i];
		vkBuffers[i] = buffer.As<VulkanBuffer>()->GetBuffer();
	}

	vkCmdBindVertexBuffers(this->m_commandBuffer, 0, nBufferCount, vkBuffers.data(), offsets.data());
}

/**
* Binds a index buffer
* 
* @param buffer Index buffer
* @param indexType Index type (default = UINT16)
*/
void 
VulkanGraphicsContext::BindIndexBuffer(Ref<GPUBuffer> buffer, EIndexType indexType = EIndexType::UINT16) {
	VkIndexType vkIndexType = VulkanHelpers::ConvertIndexType(indexType);

	VkBuffer vkBuffer = buffer.As<VulkanBuffer>()->GetBuffer();
	vkCmdBindIndexBuffer(this->m_commandBuffer, vkBuffer, 0, vkIndexType);
}
