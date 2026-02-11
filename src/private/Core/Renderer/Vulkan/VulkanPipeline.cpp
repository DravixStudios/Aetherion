#include "Core/Renderer/Vulkan/VulkanPipeline.h"

VulkanPipeline::VulkanPipeline(VkDevice device) 
	: m_device(device), 
	m_pipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE), 
	m_bindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM) { }

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

	/* Vertex input */
	Vector<VkVertexInputBindingDescription> bindings;
	for (const VertexInputBinding& binding : createInfo.vertexBindings) {
		VkVertexInputBindingDescription vkBinding = { };
		vkBinding.binding = binding.nBinding;
		vkBinding.inputRate = binding.bPerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		vkBinding.stride = binding.nStride;

		bindings.push_back(vkBinding);
	}

	Vector<VkVertexInputAttributeDescription> attributes;
	for (const VertexInputAttribute& attribute : createInfo.vertexAttributes) {
		VkVertexInputAttributeDescription vkAttribute = { };
		vkAttribute.binding = attribute.nBinding;
		vkAttribute.location = attribute.nLocation;
		vkAttribute.offset = attribute.nOffset;
		vkAttribute.format = VulkanHelpers::ConvertFormat(attribute.format);

		attributes.push_back(vkAttribute);
	}

	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.vertexAttributeDescriptionCount = attributes.size();
	vertexInfo.pVertexAttributeDescriptions = attributes.data();
	vertexInfo.vertexBindingDescriptionCount = bindings.size();
	vertexInfo.pVertexBindingDescriptions = bindings.data();

	/* Input assembly */
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { };
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VulkanHelpers::ConvertTopology(createInfo.topology);
	inputAssembly.primitiveRestartEnable = createInfo.bPrimitiveRestartEnable ? VK_TRUE : VK_FALSE;

	/* Viewport state (dynamic) */
	VkPipelineViewportStateCreateInfo viewportState = { };
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	/* Rasterizer */
	const RasterizationState& rasterState = createInfo.rasterizationState;
	VkPipelineRasterizationStateCreateInfo rasterizer = { };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = rasterState.bDepthClampEnable ? VK_TRUE : VK_FALSE;
	rasterizer.rasterizerDiscardEnable = rasterState.bRasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
	rasterizer.polygonMode = VulkanHelpers::ConvertPolygonMode(rasterState.polygonMode);
	rasterizer.lineWidth = rasterState.lineWidth;
	rasterizer.cullMode = VulkanHelpers::ConvertCullMode(rasterState.cullMode);
	rasterizer.frontFace = VulkanHelpers::ConvertFrontFace(rasterState.frontFace);
	rasterizer.depthBiasEnable = rasterState.bDepthBiasEnable ? VK_TRUE : VK_FALSE;
	rasterizer.depthBiasConstantFactor = rasterState.depthBiasConstantFactor;
	rasterizer.depthBiasClamp = rasterState.depthBiasClamp;
	rasterizer.depthBiasSlopeFactor = rasterState.depthBiasSlopeFactor;

	/* Multisampling */
	const MultisampleState& msState = createInfo.multisampleState;
	VkPipelineMultisampleStateCreateInfo multisampling = { };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = msState.bSampleShadingEnable ? VK_TRUE : VK_FALSE;
	multisampling.rasterizationSamples = static_cast<VkSampleCountFlagBits>(msState.nSampleCount);
	multisampling.minSampleShading = msState.minSampleShading;
	multisampling.alphaToCoverageEnable = msState.bAlphaToCoverageEnable ? VK_TRUE : VK_FALSE;
	multisampling.alphaToOneEnable = msState.bAlphaToOneEnable ? VK_TRUE : VK_FALSE;

	/* Depth Stencil */
	const DepthStencilState& dsState = createInfo.depthStencilState;
	VkPipelineDepthStencilStateCreateInfo depthStencil = { };
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = dsState.bDepthTestEnable ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = dsState.bDepthWriteEnable ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = VulkanHelpers::ConvertCompareOp(dsState.depthCompareOp);
	depthStencil.depthBoundsTestEnable = dsState.bDepthBoundsTestEnable ? VK_TRUE : VK_FALSE;
	depthStencil.minDepthBounds = dsState.minDepthBounds;
	depthStencil.maxDepthBounds = dsState.maxDepthBounds;
	depthStencil.stencilTestEnable = dsState.bStencilTestEnable ? VK_TRUE : VK_FALSE;
	depthStencil.front.failOp = VulkanHelpers::ConvertStencilOp(dsState.stencilFailOp);
	depthStencil.front.passOp = VulkanHelpers::ConvertStencilOp(dsState.stencilPassOp);
	depthStencil.front.depthFailOp = VulkanHelpers::ConvertStencilOp(dsState.stencilDepthFailOp);
	depthStencil.front.compareOp = VulkanHelpers::ConvertCompareOp(dsState.stencilCompareOp);
	depthStencil.front.compareMask = dsState.stencilCompareMask;
	depthStencil.front.writeMask = dsState.stencilWriteMask;
	depthStencil.front.reference = dsState.stencilReference;
	depthStencil.back = depthStencil.front;

	/* Color Blending */
	const ColorBlendState& blendState = createInfo.colorBlendState;
	Vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	for (const ColorBlendAttachment& attachment : blendState.attachments) {
		VkPipelineColorBlendAttachmentState colorBlendAttachment = { };
		colorBlendAttachment.blendEnable = attachment.bBlendEnable ? VK_TRUE : VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VulkanHelpers::ConvertBlendFactor(attachment.srcColorBlendFactor);
		colorBlendAttachment.dstColorBlendFactor = VulkanHelpers::ConvertBlendFactor(attachment.dstColorBlendFactor);
		colorBlendAttachment.colorBlendOp = VulkanHelpers::ConvertBlendOp(attachment.colorBlendOp);
		colorBlendAttachment.srcAlphaBlendFactor = VulkanHelpers::ConvertBlendFactor(attachment.srcAlphaBlendFactor);
		colorBlendAttachment.dstAlphaBlendFactor = VulkanHelpers::ConvertBlendFactor(attachment.dstAlphaBlendFactor);
		colorBlendAttachment.alphaBlendOp = VulkanHelpers::ConvertBlendOp(attachment.alphaBlendOp);

		VkColorComponentFlags colorWriteMasks = 0;
		if (attachment.bWriteR) colorWriteMasks |= VK_COLOR_COMPONENT_R_BIT;
		if (attachment.bWriteG) colorWriteMasks |= VK_COLOR_COMPONENT_G_BIT;
		if (attachment.bWriteB) colorWriteMasks |= VK_COLOR_COMPONENT_B_BIT;
		if (attachment.bWriteA) colorWriteMasks |= VK_COLOR_COMPONENT_A_BIT;
		
		colorBlendAttachment.colorWriteMask = colorWriteMasks;

		colorBlendAttachments.push_back(colorBlendAttachment);
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = { };
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = blendState.bLogicOpEnable ? VK_TRUE : VK_FALSE;
	colorBlending.attachmentCount = colorBlendAttachments.size();
	colorBlending.pAttachments = colorBlendAttachments.data();
	memcpy(colorBlending.blendConstants, blendState.blendConstants, sizeof(float) * 4);

	/* Dynamic States */
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = { };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	/* Pipeline layout */
	Ref<VulkanPipelineLayout> vkLayout = createInfo.pipelineLayout.As<VulkanPipelineLayout>();
	this->m_pipelineLayout = vkLayout->GetVkLayout();

	/* Graphics pipeline */
	VkGraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = this->m_pipelineLayout;

	if (createInfo.renderPass) {
		Ref<VulkanRenderPass> vkRenderPass = createInfo.renderPass.As<VulkanRenderPass>();
		pipelineInfo.renderPass = vkRenderPass->GetVkRenderPass();
		pipelineInfo.subpass = createInfo.nSubpass;
	}

	VK_CHECK(
		vkCreateGraphicsPipelines(
			this->m_device, 
			VK_NULL_HANDLE, 
			1, &pipelineInfo, 
			nullptr, 
			&this->m_pipeline
		), "Failed creating graphics pipeline");
}

/**
* Creates a Vulkan compute pipeline
* 
* @param createInfo Pipeline create info
*/
void 
VulkanPipeline::CreateCompute(const ComputePipelineCreateInfo& createInfo) {
	this->m_type = EPipelineType::COMPUTE;
	this->m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

	Ref<Shader> computeShader = createInfo.shader;

	VkPipelineShaderStageCreateInfo shaderStage = { };
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VulkanHelpers::ConvertSingleShaderStage(computeShader->GetStage());
	shaderStage.module = this->CreateShaderModule(computeShader->GetSPIRV());
	shaderStage.pName = "main";

	/* Pipeline layout */
	Vector<VkDescriptorSetLayout> descriptorSetLayouts;
	for (const Ref<DescriptorSetLayout>& layout : createInfo.descriptorSetLayouts) {
		Ref<VulkanDescriptorSetLayout> vkLayout = layout.As<VulkanDescriptorSetLayout>();
		descriptorSetLayouts.push_back(vkLayout->GetVkLayout());
	}

	Vector<VkPushConstantRange> pushConstantRanges;
	for (const PushConstantRange range : createInfo.pushConstantRanges) {
		VkPushConstantRange vkRange = { };
		vkRange.offset = range.nOffset;
		vkRange.size = range.nSize;
		vkRange.stageFlags = VulkanHelpers::ConvertShaderStage(range.stage);
		
		pushConstantRanges.push_back(vkRange);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { };
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	VK_CHECK(
		vkCreatePipelineLayout(
			this->m_device, 
			&pipelineLayoutInfo, 
			nullptr, 
			&this->m_pipelineLayout
		), 
		"Failed creating compute pipeline layout");

	/* Compute pipeline */
	VkComputePipelineCreateInfo pipelineInfo = { };
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shaderStage;
	pipelineInfo.layout = this->m_pipelineLayout;

	VK_CHECK(
		vkCreateComputePipelines(
			this->m_device,
			VK_NULL_HANDLE,
			1, &pipelineInfo,
			nullptr,
			&this->m_pipeline
		),
		"Failed creating compute pipeline");
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