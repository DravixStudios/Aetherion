#include "Core/Renderer/Rendering/Passes/LightingPass.h"

/**
* Lighting pass initialization
*/
void
LightingPass::Init(Ref<Device> device) {
	this->m_device = device;
}

void 
LightingPass::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;
}

/**
* Lighting pass node setup
*/
void
LightingPass::SetupNode(RenderGraphBuilder& builder) {
	builder.ReadTexture(this->m_input.albedo);
	builder.ReadTexture(this->m_input.normal);
	builder.ReadTexture(this->m_input.orm);
	builder.ReadTexture(this->m_input.emissive);
	builder.ReadTexture(this->m_input.position);

	builder.SetDimensions(this->m_nWidth, this->m_nHeight);

	TextureDesc texDesc = { };
	texDesc.format = GPUFormat::RGBA16_FLOAT;
	texDesc.nWidth = this->m_nWidth;
	texDesc.nHeight = this->m_nHeight;
	texDesc.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;

	this->m_output.hdrOutput = builder.CreateColorOutput(texDesc, EImageLayout::SHADER_READ_ONLY);
}

/**
* Executes the lighting pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
*/
void
LightingPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex) {
	context->BindPipeline(this->m_pipeline);

	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });
	
	context->PushConstants(this->m_pipelineLayout, EShaderStage::FRAGMENT, 0, sizeof(glm::vec3), &this->m_cameraPosition);

	context->BindDescriptorSets(0, { this->m_gbufferSet, this->m_lightSet });
	context->BindVertexBuffers({ this->m_vertexBuffer });

	context->BindIndexBuffer(this->m_indexBuffer);
	context->DrawIndexed(this->m_nIndexCount);
}

/**
* Sets lighting pass input
* 
* @param gbOutput G-Buffer output
*/
void
LightingPass::SetInput(const GBufferPass::Output& gbOutput) {
	this->m_input.albedo = gbOutput.albedo;
	this->m_input.normal = gbOutput.normal;
	this->m_input.orm = gbOutput.orm;
	this->m_input.emissive = gbOutput.emissive;
	this->m_input.position = gbOutput.position;
}

/**
* Sets the light data
* 
* @param lightSet Light descriptor set
* @param vertexBuffer Vertex buffer (For screenquad)
* @param indexBuffer Index bufer (For screenquad)
*/
void 
LightingPass::SetLightData(
	Ref<ImageView> irradiance,
	Ref<ImageView> prefilter,
	Ref<ImageView> brdf,
	Ref<GPUBuffer> vertexBuffer,
	Ref<GPUBuffer> indexBuffer,
	Ref<Sampler> cubeSampler,
	Ref<Sampler> linearSampler,
	uint32_t nIndexCount
) {
	this->m_irradiance = irradiance;
	this->m_prefilter = prefilter;
	this->m_brdf = brdf;
	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_cubeSampler = cubeSampler;
	this->m_linearSampler = linearSampler;
	this->m_nIndexCount = nIndexCount;

	if (this->m_device && !this->m_lightSet) {
		this->CreateDescriptorSet();
	}
}

/**
* Creates lighting descriptor sets
*/ 
void
LightingPass::SetGBufferDescriptorSet(Ref<DescriptorSet> set) {
	this->m_gbufferSet = set;
}

void
LightingPass::SetCameraPosition(const glm::vec3& position) {
	this->m_cameraPosition = position;
}

/**
* Creates lighting descriptor sets
*/
void 
LightingPass::CreateDescriptorSet() {
	/* Create descriptor set layout */
	Vector<DescriptorSetLayoutBinding> bindings = {
		{ 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 2, EShaderStage::FRAGMENT },
		{ 1, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT },
	};

	DescriptorSetLayoutCreateInfo layoutCreateInfo = { };
	layoutCreateInfo.bindings = bindings;
	layoutCreateInfo.bUpdateAfterBind = false;

	this->m_lightSetLayout = this->m_device->CreateDescriptorSetLayout(layoutCreateInfo);

	/* 
		Create descriptor pool

		3 descriptors (irradiance, prefilter, brdf) per frame
	*/
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = 1;
	poolInfo.poolSizes = {
		{ EDescriptorType::COMBINED_IMAGE_SAMPLER, 3 }
	};
	
	this->m_lightPool = this->m_device->CreateDescriptorPool(poolInfo);

	this->m_lightSet = this->m_device->CreateDescriptorSet(this->m_lightPool, this->m_lightSetLayout);

	/* Write descriptors */
	
	/* Irradiance map */
	DescriptorImageInfo irradianceInfo = { };
	irradianceInfo.imageView = this->m_irradiance;
	irradianceInfo.sampler = this->m_cubeSampler;
	irradianceInfo.texture = this->m_irradiance->GetImage();
	
	/* Prefilter map */
	DescriptorImageInfo prefilterInfo = { };
	prefilterInfo.imageView = this->m_prefilter;
	prefilterInfo.sampler = this->m_cubeSampler;
	prefilterInfo.texture = this->m_prefilter->GetImage();

	/* BRDF LUT */
	DescriptorImageInfo brdfInfo = { };
	brdfInfo.imageView = this->m_brdf;
	brdfInfo.sampler = this->m_linearSampler;
	brdfInfo.texture = this->m_brdf->GetImage();

	/* IBL Maps (Irradiance + Prefilter) */
	Vector<DescriptorImageInfo> iblInfos = { irradianceInfo, prefilterInfo };
	this->m_lightSet->WriteTextures(0, 0, iblInfos);

	this->m_lightSet->WriteTexture(1, 0, brdfInfo);

	this->m_lightSet->UpdateWrites();

	this->CreatePipeline();
}

void
LightingPass::CreatePipeline() {
	PushConstantRange pushRange = { };
	pushRange.nSize = sizeof(glm::vec3);
	pushRange.stage = EShaderStage::FRAGMENT;

	PipelineLayoutCreateInfo plInfo = { };
	plInfo.setLayouts = { this->m_gbufferSet->GetLayout(), this->m_lightSetLayout };
	plInfo.pushConstantRanges = { pushRange };

	this->m_pipelineLayout = this->m_device->CreatePipelineLayout(plInfo);

	/* Compile shaders (GLSL) */
	Ref<Shader> vertexShader = Shader::CreateShared();
	vertexShader->LoadFromGLSL("LightingPass.vert", EShaderStage::VERTEX);

	Ref<Shader> pixelShader = Shader::CreateShared();
	pixelShader->LoadFromGLSL("LightingPass.frag", EShaderStage::FRAGMENT);

	/* Create graphics pipeline */
	GraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shaders = { vertexShader, pixelShader };

	/* Vertex bindings (Screenquad) */
	pipelineInfo.vertexBindings = {
		{ 0, sizeof(ScreenQuadVertex), false} // Per vertex
	};

	pipelineInfo.vertexAttributes = {
		{ 0, 0, GPUFormat::RGB32_FLOAT, offsetof(ScreenQuadVertex, position)},
		{ 1, 0, GPUFormat::RG32_FLOAT, offsetof(ScreenQuadVertex, texCoord)}
	};

	/* Rasterization */
	pipelineInfo.rasterizationState.cullMode = ECullMode::BACK;
	pipelineInfo.rasterizationState.frontFace = EFrontFace::COUNTER_CLOCKWISE;

	/* Depth (Disable depth test for fullscreen quad) */
	pipelineInfo.depthStencilState.bDepthTestEnable = false;
	pipelineInfo.depthStencilState.bDepthWriteEnable = false;

	/* Color blend */
	ColorBlendAttachment colorBlend = { };
	colorBlend.bBlendEnable = false; 
	colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;

	ColorBlendState colorBlendState = { };
	colorBlendState.attachments = { colorBlend };

	pipelineInfo.colorBlendState = colorBlendState;

	/* Create a compatible render pass */
	/* TODO: Use dynamic rendering */
	RenderPassCreateInfo rpInfo = { };

	/* Attachment 0: HDR Output - RGBA16_FLOAT */
	AttachmentDescription colorAttachment = { };
	colorAttachment.format = GPUFormat::RGBA16_FLOAT;
	colorAttachment.sampleCount = ESampleCount::SAMPLE_1;
	colorAttachment.initialLayout = EImageLayout::UNDEFINED;
	colorAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	colorAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	colorAttachment.storeOp = EAttachmentStoreOp::STORE;
	colorAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	colorAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	rpInfo.attachments = { colorAttachment };

	SubpassDescription subpass = { };
	subpass.colorAttachments = { { 0, EImageLayout::COLOR_ATTACHMENT } };
	rpInfo.subpasses = { subpass };

	Ref<RenderPass> renderPass = this->m_device->CreateRenderPass(rpInfo);

	/* Set color formats for Dynamic rendering */
	pipelineInfo.colorFormats = { GPUFormat::RGBA16_FLOAT };

	pipelineInfo.renderPass = renderPass;
	pipelineInfo.pipelineLayout = this->m_pipelineLayout;
	pipelineInfo.nSubpass = 0;

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);
}