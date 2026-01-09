#include "Core/Renderer/Vulkan/VulkanPipeline.h"

VulkanPipeline::VulkanPipeline(VkDevice device) 
	: m_device(device), 
	m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE) { }

VulkanPipeline::~VulkanPipeline() {
	if (this->m_pipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(this->m_device, this->m_pipeline, nullptr);
	}

	if (this->m_pipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(this->m_device, this->m_pipelineLayout, nullptr);
	}
}

/**
* Creates a Vulkan graphics pipeline
* 
* @param createInfo Pipeline create info
*/
void 
VulkanPipeline::CreateGraphics(const GraphicsPipelineCreateInfo& createInfo) {
	this->m_type = EPipelineType::GRAPHICS;
	this->m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	/* Shader stages */
	Vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const Ref<Shader>& shader : createInfo.shaders) {
		VkPipelineShaderStageCreateInfo stageInfo = { };
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = VulkanHelpers::ConvertSingleShaderStage(shader->GetStage());
		stageInfo.module = this->CreateShaderModule(shader->GetSPIRV());
		stageInfo.pName = "main";
		shaderStages.push_back(stageInfo);
	}


}

/**
* Creates a Vulkan compute pipeline
* 
* @param createInfo Pipeline create info
*/
void 
VulkanPipeline::CreateCompute(const ComputePipelineCreateInfo& createInfo) {

}

/**
* Creates a Vulkan shader module
* 
* @param shaderCode Shader SPIR-V Code
* 
* @returns Created shader module
*/
VkShaderModule 
VulkanPipeline::CreateShaderModule(const Vector<uint32_t>& shaderCode) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
	createInfo.pCode = shaderCode.data();

	VkShaderModule module = nullptr;
	VK_CHECK(vkCreateShaderModule(this->m_device, &createInfo, nullptr, &module), "Failed creating shader module");

	return module;
}