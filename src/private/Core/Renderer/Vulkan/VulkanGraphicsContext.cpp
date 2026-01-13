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
