#include "Core/Renderer/Rendering/Passes/SkyboxPass.h"

/**
* Skybox pass initialization
*/
void 
SkyboxPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Skybox node setup
*/
void 
SkyboxPass::SetupNode(RenderGraphBuilder& builder) {
	builder.ReadTexture(this->m_input.depth);
	builder.UseColorOutput(this->m_input.hdrOutput);
}

/**
* Executes skybox pass node
*/
void
SkyboxPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
	context->BindPipeline(this->m_pipeline);

	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };

	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });

	context->BindDescriptorSets(0, { this->m_skyboxSet });
	context->BindVertexBuffers({ this->m_vertexBuffer });
	context->BindIndexBuffer(this->m_indexBuffer);

	context->DrawIndexed(this->m_nIndexCount, 1, 0, 0, 0);
}

/**
* Sets skybox pass data
*/
void
SkyboxPass::SetSkyboxData(
	Ref<DescriptorSet> skyboxSet,
	Ref<DescriptorSetLayout> skyboxSetLayout,
	Ref<GPUBuffer> vertexBuffer,
	Ref<GPUBuffer> indexBuffer,
	uint32_t nIndexCount
) {
	this->m_skyboxSet = skyboxSet;
	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_nIndexCount = nIndexCount;

	if (this->m_device && !this->m_pipeline) {
		this->CreatePipeline();
	}
}

/**
* Creates skybox pipeline
*/
void
SkyboxPass::CreatePipeline() {
	PipelineLayoutCreateInfo plInfo = { };
	plInfo.setLayouts = { this->m_skyboxSetLayout };
	
	this->m_pipelineLayout = this->m_device->CreatePipelineLayout(plInfo);

	/* Compile shaders */
	Ref<Shader> vertexShader = Shader::CreateShared();
	vertexShader->LoadFromGLSL("SkyboxPass.vert", EShaderStage::VERTEX);

	Ref<Shader> pixelShader = Shader::CreateShared();
	pixelShader->LoadFromGLSL("SkyboxPass.frag", EShaderStage::FRAGMENT);

	/* Create graphics pipeline */
	GraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shaders = { vertexShader, pixelShader };

	/* Vertex bindings */
	pipelineInfo.vertexBindings = {
		{ 0, sizeof(ScreenQuadVertex), false }
	};

	pipelineInfo.vertexAttributes = {
		{ 0, 0, GPUFormat::RGB32_FLOAT, offsetof(ScreenQuadVertex, position)},
		{ 1, 0, GPUFormat::RG32_FLOAT, offsetof(ScreenQuadVertex, texCoord)}
	};

	/* Rasterization */
	pipelineInfo.rasterizationState.cullMode = ECullMode::NONE;
	pipelineInfo.rasterizationState.frontFace = EFrontFace::COUNTER_CLOCKWISE;

	/* Depth */
	pipelineInfo.depthStencilState.bDepthTestEnable = false;
	pipelineInfo.depthStencilState.bDepthWriteEnable = false;
	pipelineInfo.depthStencilState.depthCompareOp = ECompareOp::NEVER;

	/* Color blend (1 attachment, no blending) */
	ColorBlendAttachment colorBlend = { };
	colorBlend.bBlendEnable = false;
	colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;
	
	ColorBlendState colorBlendState = { };
	colorBlendState.attachments.push_back(colorBlend);

	pipelineInfo.colorBlendState = colorBlendState;

	/* Create a compatible render pass */
	RenderPassCreateInfo rpInfo = { };

	AttachmentDescription colorAttachment = { };
	colorAttachment.format = GPUFormat::RGBA16_FLOAT;
	colorAttachment.sampleCount = ESampleCount::SAMPLE_1;
	colorAttachment.initialLayout = EImageLayout::COLOR_ATTACHMENT;
	colorAttachment.finalLayout = EImageLayout::COLOR_ATTACHMENT;
	colorAttachment.loadOp = EAttachmentLoadOp::LOAD;
	colorAttachment.storeOp = EAttachmentStoreOp::STORE;
	colorAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	colorAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
	
	rpInfo.attachments.push_back(colorAttachment);
	
	SubpassDescription subpass = { };
	subpass.colorAttachments = {
		{ 0, EImageLayout::COLOR_ATTACHMENT }
	};
	subpass.bHasDepthStencil = false;

	rpInfo.subpasses = { subpass };

	this->m_compatRenderPass = this->m_device->CreateRenderPass(rpInfo);

	pipelineInfo.colorFormats = {
		GPUFormat::RGBA16_FLOAT
	};

	pipelineInfo.renderPass = this->m_compatRenderPass;
	pipelineInfo.nSubpass = 0;

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);
}