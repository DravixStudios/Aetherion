#include "Core/Renderer/Rendering/Passes/GBufferPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Core/Renderer/Shader.h"

/**
* G-Buffer pass initialization
* @param device Logical device
*/
void
GBufferPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Setup the node
* 
* @param builder Render graph builder
*/
void
GBufferPass::SetupNode(RenderGraphBuilder& builder) {
	TextureDesc colorDesc = { };
	colorDesc.nWidth = this->m_nWidth;
	colorDesc.nHeight = this->m_nHeight;
	colorDesc.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;

	/* Albedo */
	colorDesc.format = GBufferLayout::ALBEDO;
	this->m_output.albedo = builder.CreateColorOutput(colorDesc);

	/* Normal */
	colorDesc.format = GBufferLayout::NORMAL;
	this->m_output.normal = builder.CreateColorOutput(colorDesc);

	/* ORM */
	colorDesc.format = GBufferLayout::ORM;
	this->m_output.orm = builder.CreateColorOutput(colorDesc);

	/* Emissive */
	colorDesc.format = GBufferLayout::EMISSIVE;
	this->m_output.emissive = builder.CreateColorOutput(colorDesc);

	/* Position */
	colorDesc.format = GBufferLayout::POSITION;
	this->m_output.position = builder.CreateColorOutput(colorDesc);
	
	/* Depth */
	TextureDesc depthDesc = { };
	depthDesc.format = GBufferLayout::DEPTH;
	depthDesc.nWidth = this->m_nWidth;
	depthDesc.nHeight = this->m_nHeight;
	depthDesc.usage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;
	this->m_output.depth = builder.CreateDepthOutput(depthDesc);
}

/**
* Executes the G-Buffer pass
*/
void
GBufferPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
	context->BindPipeline(this->m_pipeline);
	
	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });

	context->BindDescriptorSets(0, { this->m_sceneSet });
	context->BindVertexBuffers({ this->m_vertexBuffer });
	context->BindIndexBuffer({ this->m_indexBuffer });
	context->DrawIndexed(this->m_nIndexCount);
}

/**
* Sets Scene data
* 
* @param sceneSet Scene Descriptor set
* @param sceneSetLayout Scene descriptor set layout
* @param bindlessSet Bindless descriptor set
* @param bindlessSetLayout Bindless descriptor set layout
* @param vertexBuffer Scene vertex buffer
* @param indexBuffer Scene index buffer
* @param nIndexCount Scene index count
*/
void 
GBufferPass::SetSceneData(
	Ref<DescriptorSet> sceneSet,
	Ref<DescriptorSetLayout> sceneSetLayout,
	Ref<DescriptorSet> bindlessSet,
	Ref<DescriptorSetLayout> bindlessSetLayout,
	Ref<GPUBuffer> vertexBuffer,
	Ref<GPUBuffer> indexBuffer,
	uint32_t nIndexCount
) {
	this->m_sceneSet = sceneSet;
	this->m_sceneSetLayout = sceneSetLayout;

	this->m_bindlessSet = bindlessSet;
	this->m_bindlessSetLayout = bindlessSetLayout;

	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_nIndexCount = nIndexCount;

	if (this->m_device && !this->m_pipeline) {
		this->CreatePipeline();
	}
}

void
GBufferPass::CreatePipeline() {
	PipelineLayoutCreateInfo plInfo = { };
	plInfo.setLayouts = { this->m_sceneSetLayout, this->m_bindlessSetLayout };
	plInfo.pushConstantRanges = {
		{ EShaderStage::VERTEX, 0, sizeof(uint32_t)} // wvpAlignment
	};

	this->m_pipelineLayout = this->m_device->CreatePipelineLayout(plInfo);

	/* Compile shaders (GLSL) */
	Ref<Shader> vertexShader = Shader::CreateShared();
	vertexShader->LoadFromGLSL("GBufferPass.vert", EShaderStage::VERTEX);

	Ref<Shader> pixelShader = Shader::CreateShared();
	vertexShader->LoadFromGLSL("GBufferPass.frag", EShaderStage::FRAGMENT);

	/* Create graphics pipeline */
	GraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shaders = { vertexShader, pixelShader };

	/* Vertex bindings */
	pipelineInfo.vertexBindings = {
		{ 0, sizeof(Vertex), false} // Per vertex
	};

	pipelineInfo.vertexAttributes = {
		{ 0, 0, GPUFormat::RGB32_FLOAT, offsetof(Vertex, position)},
		{ 1, 0, GPUFormat::RGB32_FLOAT, offsetof(Vertex, normal)},
		{ 2, 0, GPUFormat::RGB32_FLOAT, offsetof(Vertex, texCoord)}
	};

	/* Rasterization */
	pipelineInfo.rasterizationState.cullMode = ECullMode::BACK;
	pipelineInfo.rasterizationState.frontFace = EFrontFace::COUNTER_CLOCKWISE;

	/* Depth */
	pipelineInfo.depthStencilState.bDepthTestEnable = true;
	pipelineInfo.depthStencilState.bDepthWriteEnable = true;
	pipelineInfo.depthStencilState.depthCompareOp = ECompareOp::LESS;

	/* Color blend (5 attachments, no blending) */
	ColorBlendAttachment colorBlend = { };
	colorBlend.bBlendEnable = false;
	colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;

	Vector<ColorBlendAttachment> colorBlendAttachments = {
		colorBlend, colorBlend, colorBlend, colorBlend, colorBlend
	};
	
	ColorBlendState colorBlendState = { };
	colorBlendState.attachments = colorBlendAttachments;

	pipelineInfo.colorBlendState = colorBlendState;

	/* Create a compatible render pass */
	/* TODO: Use dynamic rendering */
	RenderPassCreateInfo rpInfo = { };

	/* Attachment 0: Base color - RGBA16_FLOAT */
	AttachmentDescription colorAttachment = { };
	colorAttachment.format = GBufferLayout::ALBEDO;
	colorAttachment.sampleCount = ESampleCount::SAMPLE_1;
	colorAttachment.initialLayout = EImageLayout::UNDEFINED;
	colorAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	colorAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	colorAttachment.storeOp = EAttachmentStoreOp::STORE;
	colorAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	colorAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	/* Attachment 1: Normal - RGBA16_FLOAT */
	AttachmentDescription normalAttachment = { };
	normalAttachment.format = GBufferLayout::NORMAL;
	normalAttachment.sampleCount = ESampleCount::SAMPLE_1;
	normalAttachment.initialLayout = EImageLayout::UNDEFINED;
	normalAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	normalAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	normalAttachment.storeOp = EAttachmentStoreOp::STORE;
	normalAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	normalAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	/* Attachment 2: ORM - RGBA16_FLOAT */
	AttachmentDescription ormAttachment = { };
	ormAttachment.format = GBufferLayout::ORM;
	ormAttachment.sampleCount = ESampleCount::SAMPLE_1;
	ormAttachment.initialLayout = EImageLayout::UNDEFINED;
	ormAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	ormAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	ormAttachment.storeOp = EAttachmentStoreOp::STORE;
	ormAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	ormAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	/* Attachment 3: Emissive - RGBA16_FLOAT */
	AttachmentDescription emissiveAttachment = { };
	emissiveAttachment.format = GBufferLayout::EMISSIVE;
	emissiveAttachment.sampleCount = ESampleCount::SAMPLE_1;
	emissiveAttachment.initialLayout = EImageLayout::UNDEFINED;
	emissiveAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	emissiveAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	emissiveAttachment.storeOp = EAttachmentStoreOp::STORE;
	emissiveAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	emissiveAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	/* Attachment 4: Position - RGBA16_FLOAT */
	AttachmentDescription positionAttachment = { };
	positionAttachment.format = GBufferLayout::POSITION;
	positionAttachment.sampleCount = ESampleCount::SAMPLE_1;
	positionAttachment.initialLayout = EImageLayout::UNDEFINED;
	positionAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	positionAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	positionAttachment.storeOp = EAttachmentStoreOp::STORE;
	positionAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	positionAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	/* Attachment 5: Depth */
	AttachmentDescription depthAttachment = { };
	depthAttachment.format = GBufferLayout::DEPTH;
	depthAttachment.sampleCount = ESampleCount::SAMPLE_1;
	depthAttachment.initialLayout = EImageLayout::UNDEFINED;
	depthAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	depthAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	depthAttachment.storeOp = EAttachmentStoreOp::STORE;
	depthAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	depthAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;

	Vector<AttachmentDescription> attachments = {
		colorAttachment, normalAttachment,
		ormAttachment, emissiveAttachment,
		positionAttachment, depthAttachment
	};

	rpInfo.attachments = attachments;

	SubpassDescription subpass = { };
	subpass.colorAttachments = {
		{ 0, EImageLayout::COLOR_ATTACHMENT },
		{ 1, EImageLayout::COLOR_ATTACHMENT },
		{ 2, EImageLayout::COLOR_ATTACHMENT },
		{ 3, EImageLayout::COLOR_ATTACHMENT },
		{ 4, EImageLayout::COLOR_ATTACHMENT }
	};
	subpass.bHasDepthStencil = true;
	subpass.depthStencilAttachment = { 5, EImageLayout::DEPTH_STENCIL_ATTACHMENT };
	rpInfo.subpasses = { subpass };

	this->m_compatRenderPass = this->m_device->CreateRenderPass(rpInfo);

	/* Set color formats for Dynamic rendering */
	pipelineInfo.colorFormats = {
		GBufferLayout::ALBEDO,
		GBufferLayout::NORMAL,
		GBufferLayout::ORM,
		GBufferLayout::EMISSIVE,
		GBufferLayout::POSITION
	};

	pipelineInfo.depthFormat = GBufferLayout::DEPTH;
	pipelineInfo.renderPass = this->m_compatRenderPass;
	pipelineInfo.nSubpass = 0;

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);
}