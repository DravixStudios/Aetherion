#include "Core/Renderer/Rendering/Passes/TonemapPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"

/**
* Initializes the pass (Base implementation)
*/
void
TonemapPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Initializes the pass
*/
void 
TonemapPass::Init(Ref<Device> device, Ref<Swapchain> swapchain, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;

	/* Create sampler */
	SamplerCreateInfo samplerInfo = { };
	samplerInfo.magFilter = EFilter::LINEAR;
	samplerInfo.minFilter = EFilter::LINEAR;
	samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeV = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeW = EAddressMode::CLAMP_TO_EDGE;

	this->m_sampler = device->CreateSampler(samplerInfo);

	this->CreateDescriptorSets();
	this->CreatePipeline(GPUFormat::BGRA8_UNORM);
}

/**
* Setups the node
*/
void 
TonemapPass::SetupNode(RenderGraphBuilder& builder) {
	builder.ReadTexture(this->m_input);
	builder.UseColorOutput(this->m_output, EImageLayout::PRESENT_SRC);
	
	builder.SetDimensions(this->m_nWidth, this->m_nHeight);
}

/**
* Executes the pass
*/
void 
TonemapPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex) {
	/* Get the actual view for the input texture */
	Ref<ImageView> inputView = graphCtx.GetImageView(this->m_input);
	
	/* Update descriptor set with the current view */
	this->UpdateDescriptorSet(nFrameIndex, inputView);

	context->BindPipeline(this->m_pipeline);

	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });

	context->BindDescriptorSets(0, { this->m_sets[nFrameIndex] });
	context->BindVertexBuffers({ this->m_vertexBuffer });
	context->BindIndexBuffer(this->m_indexBuffer);

	context->DrawIndexed(this->m_nIndexCount, 1, 0, 0, 0);
}

void 
TonemapPass::SetInput(TextureHandle input) {
	this->m_input = input;
}

void 
TonemapPass::SetOutput(TextureHandle output) {
	this->m_output = output;
}

void 
TonemapPass::SetScreenQuad(Ref<GPUBuffer> vertexBuffer, Ref<GPUBuffer> indexBuffer, uint32_t nIndexCount) {
	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_nIndexCount = nIndexCount;
}

void 
TonemapPass::CreateDescriptorSets() {
	/* Layout: Binding 0 = Combined Image Sampler */
	DescriptorSetLayoutBinding binding = { 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT };
	
	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = { binding };
	
	this->m_setLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Pool */
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = this->m_nFramesInFlight;
	poolInfo.poolSizes = { { EDescriptorType::COMBINED_IMAGE_SAMPLER, this->m_nFramesInFlight } };
	
	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Sets */
	this->m_sets.resize(this->m_nFramesInFlight);
	for (uint32_t i = 0; i < this->m_nFramesInFlight; i++) {
		this->m_sets[i] = this->m_device->CreateDescriptorSet(this->m_pool, this->m_setLayout);
	}
}

void 
TonemapPass::UpdateDescriptorSet(uint32_t nFrameIndex, Ref<ImageView> inputView) {
	DescriptorImageInfo imageInfo = { };
	imageInfo.imageView = inputView;
	imageInfo.sampler = this->m_sampler;
	imageInfo.texture = inputView->GetImage(); 

	this->m_sets[nFrameIndex]->WriteTexture(0, 0, imageInfo);
	this->m_sets[nFrameIndex]->UpdateWrites();
}

void 
TonemapPass::CreatePipeline(GPUFormat format) {
	/* Shaders */
	Ref<Shader> vert = Shader::CreateShared();
	vert->LoadFromGLSL("TonemapPass.vert", EShaderStage::VERTEX);
	
	Ref<Shader> frag = Shader::CreateShared();
	frag->LoadFromGLSL("TonemapPass.frag", EShaderStage::FRAGMENT);

	GraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shaders = { vert, frag };

	/* Vertex Input */
	pipelineInfo.vertexBindings = {
		{ 0, sizeof(ScreenQuadVertex), false }
	};
	pipelineInfo.vertexAttributes = {
		{ 0, 0, GPUFormat::RGB32_FLOAT, offsetof(ScreenQuadVertex, position)},
		{ 1, 0, GPUFormat::RG32_FLOAT, offsetof(ScreenQuadVertex, texCoord)}
	};

	/* Input Assembly etc. */
	pipelineInfo.rasterizationState.cullMode = ECullMode::NONE;
	pipelineInfo.rasterizationState.frontFace = EFrontFace::COUNTER_CLOCKWISE;
	
	pipelineInfo.depthStencilState.bDepthTestEnable = false;
	pipelineInfo.depthStencilState.bDepthWriteEnable = false;

	/* Blending - Disabled, we overwrite backbuffer */
	ColorBlendAttachment colorBlend = { };
	colorBlend.bBlendEnable = false;
	colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;
	
	pipelineInfo.colorBlendState.attachments = { colorBlend };

	/* Layout */
	PipelineLayoutCreateInfo plInfo = { };
	plInfo.setLayouts = { this->m_setLayout };
	this->m_pipelineLayout = this->m_device->CreatePipelineLayout(plInfo);

	pipelineInfo.pipelineLayout = this->m_pipelineLayout;

	/* Compatible RenderPass */
	RenderPassCreateInfo rpInfo = { };
	AttachmentDescription colorAttachment = { };
	colorAttachment.format = format; 
	colorAttachment.sampleCount = ESampleCount::SAMPLE_1;
	colorAttachment.initialLayout = EImageLayout::UNDEFINED;
	colorAttachment.finalLayout = EImageLayout::PRESENT_SRC; // Final layout for presentation
	colorAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	colorAttachment.storeOp = EAttachmentStoreOp::STORE;
	
	rpInfo.attachments = { colorAttachment };
	
	SubpassDescription subpass = { };
	subpass.colorAttachments = { { 0, EImageLayout::COLOR_ATTACHMENT } };
	rpInfo.subpasses = { subpass };

	this->m_compatRenderPass = this->m_device->CreateRenderPass(rpInfo);
	
	pipelineInfo.renderPass = this->m_compatRenderPass;
	pipelineInfo.nSubpass = 0;
	
	/* Dynamic Rendering Color Formats if used */
	pipelineInfo.colorFormats = { format }; 

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);
}
