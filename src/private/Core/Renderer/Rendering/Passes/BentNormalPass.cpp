#include "Core/Renderer/Rendering/Passes/BentNormalPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"

/**
* Bent normal pass initialization
* 
* @param device Logical device
*/
void
BentNormalPass::Init(Ref<Device> device) {
	this->m_device = device;
}

void 
BentNormalPass::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;

	/* Create a linear sampler */
	SamplerCreateInfo samplerInfo = { };
	samplerInfo.addressModeU = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeV = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeW = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.bAnisotropyEnable = true;
	samplerInfo.maxAnisotropy = 16.f;
	samplerInfo.borderColor = EBorderColor::FLOAT_OPAQUE_WHITE;
	samplerInfo.bUnnormalizedCoordinates = false;
	samplerInfo.bCompareEnable = false;
	samplerInfo.compareOp = ECompareOp::NEVER;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;
	samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_LINEAR;

	this->m_sampler = this->m_device->CreateSampler(samplerInfo);

	/* Create camera ring buffer */
	uint32_t nCameraAlignment = NextPowerOf2(sizeof(CameraData));

	RingBufferCreateInfo cameraInfo = { };
	cameraInfo.nAlignment = nCameraAlignment;
	cameraInfo.nBufferSize = nCameraAlignment * this->m_nFramesInFlight;
	cameraInfo.nFramesInFlight = this->m_nFramesInFlight;
	cameraInfo.usage = EBufferUsage::UNIFORM_BUFFER;

	this->m_cameraBuff = this->m_device->CreateRingBuffer(cameraInfo);
}

/**
* Bent normal pass node setup
* 
* @param builder Render graph builder
*/
void 
BentNormalPass::SetupNode(RenderGraphBuilder& builder) {
	builder.SetDimensions(this->m_nWidth, this->m_nHeight);
	builder.UseColorOutput(this->m_output, EImageLayout::SHADER_READ_ONLY);
}

/**
* Set bent normal pass input
* 
* @param normal Normal G-Buffer
* @param depth Depth buffer
*/
void
BentNormalPass::SetInput(TextureHandle normal, TextureHandle depth) {
	this->m_input = { normal, depth };
}

void
BentNormalPass::SetOutput(TextureHandle bentNormal) {
	this->m_output = bentNormal;
}

/**
* Executes the bent normal pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
* @param nFrameIdx Frame index
*/
void
BentNormalPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nImgIdx) {
	if (!this->m_pool) this->CreateDescriptors();
	if (!this->m_pipeline) this->CreatePipeline();
	
	Ref<GPUTexture> normal = graphCtx.GetTexture(this->m_input.normal);
	Ref<GPUTexture> depth = graphCtx.GetTexture(this->m_input.depth);

	Ref<ImageView> normalView = graphCtx.GetImageView(this->m_input.normal);
	Ref<ImageView> depthView = graphCtx.GetImageView(this->m_input.depth);
	
	DescriptorImageInfo normalInfo = { };
	normalInfo.sampler = this->m_sampler;
	normalInfo.imageView = normalView;
	normalInfo.texture = normal;

	DescriptorImageInfo depthInfo = { };
	depthInfo.sampler = this->m_sampler;
	depthInfo.imageView = depthView;
	depthInfo.texture = depth;

	Ref<DescriptorSet> descriptorSet = this->m_descriptorSets[nImgIdx];

	descriptorSet->WriteTexture(0, 0, normalInfo);
	descriptorSet->WriteTexture(1, 0, depthInfo);
	descriptorSet->UpdateWrites();

	/* Update camera data */
	this->m_cameraBuff->Reset(nImgIdx);
	
	CameraData cameraData = { };
	cameraData.inverseView = glm::affineInverse(this->m_view);
	cameraData.inverseProjection = glm::inverse(this->m_projection);
	cameraData.projection = this->m_projection;
	cameraData.radius = 1.f;

	uint32_t nCamDataOffset = 0;
	void* pCamData = this->m_cameraBuff->Allocate(NextPowerOf2(sizeof(cameraData)), nCamDataOffset);

	memcpy(pCamData, &cameraData, sizeof(cameraData));

	/* Render */
	Viewport vp{ 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });
	context->BindPipeline(this->m_pipeline);
	context->BindDescriptorSets(0, { descriptorSet, this->m_cameraSet }, { nCamDataOffset });

	context->BindVertexBuffers({ this->m_sqVBO });
	context->BindIndexBuffer(this->m_sqIBO);

	context->DrawIndexed(this->m_nIndexCount);
}

/**
* Sets the screen quad for the bent
* normal pass
* 
* @param sqVBO Screen quad vertex buffer
* @param sqIBO Screen quad index buffer
* @param nIndexCount Index count
*/
void 
BentNormalPass::SetScreenQuad(Ref<GPUBuffer> sqVBO, Ref<GPUBuffer> sqIBO, uint32_t nIndexCount) {
	this->m_sqVBO = sqVBO;
	this->m_sqIBO = sqIBO;
	this->m_nIndexCount = nIndexCount;
}

/**
* Create bent normal pass pipeline
*/
void 
BentNormalPass::CreatePipeline() {
	/* Compile shaders */
	Ref<Shader> VShader = Shader::CreateShared();
	Ref<Shader> PShader = Shader::CreateShared();

	VShader->LoadFromGLSL("BentNormal.vert", EShaderStage::VERTEX);
	PShader->LoadFromGLSL("BentNormal.frag", EShaderStage::FRAGMENT);
	
	/* Create pipeline layout */
	PipelineLayoutCreateInfo layoutInfo = { };
	layoutInfo.setLayouts = Vector{ this->m_setLayout, this->m_cameraSetLayout };

	this->m_layout = this->m_device->CreatePipelineLayout(layoutInfo);

	/* Create compatible render pass */
	AttachmentDescription attachment;
	attachment.initialLayout = EImageLayout::UNDEFINED;
	attachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	attachment.loadOp = EAttachmentLoadOp::CLEAR;
	attachment.storeOp = EAttachmentStoreOp::STORE;
	attachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	attachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
	attachment.format = GBufferLayout::BENT_NORMAL;
	attachment.sampleCount = ESampleCount::SAMPLE_1;

	SubpassDescription subpass = { };
	subpass.colorAttachments = { { 0, EImageLayout::COLOR_ATTACHMENT } };

	RenderPassCreateInfo rpInfo = { };
	rpInfo.attachments = Vector{ attachment };
	rpInfo.subpasses = Vector{ subpass };
	
	this->m_renderPass = this->m_device->CreateRenderPass(rpInfo);

	/* Multisampling */
	MultisampleState ms = { };
	ms.nSampleCount = 1;

	/* Rasterization */
	RasterizationState raster = { };
	raster.cullMode = ECullMode::NONE;
	raster.frontFace = EFrontFace::COUNTER_CLOCKWISE;
	raster.polygonMode = EPolygonMode::FILL;

	/* Depth stencil */
	DepthStencilState dsState = { };
	dsState.bDepthTestEnable = false;
	dsState.bStencilTestEnable = false;
	dsState.bDepthWriteEnable = false;

	/* Color blend */
	ColorBlendAttachment blendAttachment = { };
	blendAttachment.bBlendEnable = false;
	blendAttachment.bWriteR = blendAttachment.bWriteG = blendAttachment.bWriteB = blendAttachment.bWriteA = true;

	ColorBlendState colorBlend = { };
	colorBlend.attachments = { blendAttachment };

	/* Create graphics pipeline */
	GraphicsPipelineCreateInfo plInfo = { };
	plInfo.nSubpass = 0;
	plInfo.pipelineLayout = this->m_layout;
	plInfo.renderPass = this->m_renderPass;
	plInfo.colorFormats = Vector{ GBufferLayout::BENT_NORMAL };
	plInfo.multisampleState = ms;
	plInfo.rasterizationState = raster;
	plInfo.depthStencilState = dsState;
	plInfo.shaders = Vector{ VShader, PShader };
	plInfo.colorBlendState = colorBlend;
	plInfo.colorFormats = Vector{ GBufferLayout::BENT_NORMAL };
	plInfo.vertexAttributes = {
		{ 0, 0, GPUFormat::RGB32_FLOAT, offsetof(ScreenQuadVertex, position)},
		{ 1, 0, GPUFormat::RG32_FLOAT, offsetof(ScreenQuadVertex, texCoord)},
	};
	plInfo.vertexBindings = {
		{ 0, sizeof(ScreenQuadVertex), false}
	};
	plInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(plInfo);
}

void
BentNormalPass::SetCameraData(glm::mat4 view, glm::mat4 projection) {
	this->m_view = view;
	this->m_projection = projection;
}

/**
* Create bent normal pass descriptors
*/
void
BentNormalPass::CreateDescriptors() {
	Vector<DescriptorSetLayoutBinding> bindings = {
		{ 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT },
		{ 1, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT }
	};
	
	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = bindings;

	this->m_setLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Create descriptor pool */
	DescriptorPoolSize poolSize = { };
	poolSize.nDescriptorCount = 2 * this->m_nFramesInFlight;
	poolSize.type = EDescriptorType::COMBINED_IMAGE_SAMPLER;

	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = this->m_nFramesInFlight;
	poolInfo.poolSizes = Vector{ poolSize };

	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Allocate descriptor sets */
	
	this->m_descriptorSets.resize(this->m_nFramesInFlight);

	for (uint32_t i = 0; i < this->m_nFramesInFlight; i++) {
		this->m_descriptorSets[i] = this->m_device->CreateDescriptorSet(this->m_pool, this->m_setLayout);
	}

	/* Create camera descriptor */
	Vector<DescriptorSetLayoutBinding> cameraBindings = {
		{ 0, EDescriptorType::UNIFORM_BUFFER_DYNAMIC, 1, EShaderStage::FRAGMENT }
	};

	DescriptorSetLayoutCreateInfo cameraLayoutInfo = { };
	cameraLayoutInfo.bindings = cameraBindings;

	this->m_cameraSetLayout = this->m_device->CreateDescriptorSetLayout(cameraLayoutInfo);

	DescriptorPoolSize cameraPoolSize = { };
	cameraPoolSize.nDescriptorCount = 1;
	cameraPoolSize.type = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;
	
	DescriptorPoolCreateInfo cameraPoolInfo = { };
	cameraPoolInfo.nMaxSets = 1;
	cameraPoolInfo.poolSizes = Vector{ cameraPoolSize };

	this->m_cameraPool = this->m_device->CreateDescriptorPool(cameraPoolInfo);

	this->m_cameraSet = this->m_device->CreateDescriptorSet(this->m_cameraPool, this->m_cameraSetLayout);

	DescriptorBufferInfo cameraInfo = { };
	cameraInfo.buffer = this->m_cameraBuff->GetBuffer();
	cameraInfo.nOffset = 0;
	cameraInfo.nRange = sizeof(CameraData);

	this->m_cameraSet->WriteBuffer(0, 0, cameraInfo);
	this->m_cameraSet->UpdateWrites();
}