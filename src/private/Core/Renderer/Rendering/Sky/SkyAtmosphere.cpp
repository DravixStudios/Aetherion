#include "Core/Renderer/Rendering/Sky/SkyAtmosphere.h"

/**
* Initialize the sky atmosphere
* 
* @param device Logical device
* @param nFramesInFlight Frames in flight count
*/
void
SkyAtmosphere::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;

	this->CreateResources();
	this->CreateDescriptors();
	this->CreatePipeline();
}

/**
* Sky atmosphere update
* 
* @param context Graphics context
* @param nFrameIdx Current frame index
*/
void
SkyAtmosphere::Update(Ref<GraphicsContext> context, uint32_t nFrameIdx) {
    this->m_sunDataBuff->Reset(nFrameIdx);
    this->m_camDataBuff->Reset(nFrameIdx);

    SunData sunData = { };
    sunData.sunDir = this->m_sunDir;
    sunData.sunHeight = -sunData.sunDir.y;

    uint32_t nSunDataOffset = 0;
    void* pSunData = this->m_sunDataBuff->Allocate(sizeof(sunData), nSunDataOffset);
    memcpy(pSunData, &sunData, sizeof(SunData));

    Viewport viewport = { };
    viewport.width = static_cast<float>(SKYBOX_FACE_SIZE);
    viewport.height = -static_cast<float>(SKYBOX_FACE_SIZE);
    viewport.x = 0.f;
    viewport.y = static_cast<float>(SKYBOX_FACE_SIZE);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    Rect2D scissor = { };
    scissor.extent = { SKYBOX_FACE_SIZE, SKYBOX_FACE_SIZE };
    scissor.offset = { 0, 0 };

    ClearValue clearValue{ { 0.f, 0.f, 0.f, 1.f } };

    /* Cubemap capture matrices (same as IBLGenerator) */
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.f, .1f, 10.f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)), // +X
        glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)), // -X
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)), // +Y
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)), // -Y
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)), // +Z
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f)) // -Z
    };

    for (uint32_t nFace = 0; nFace < 6; nFace++) {
        /* Allocate per-face camera data from ring buffer */
        CameraData camData = { };
        camData.view = captureViews[nFace];
        camData.proj = captureProjection;

        uint32_t nCamDataOffset = 0;
        void* pCamData = this->m_camDataBuff->Allocate(sizeof(camData), nCamDataOffset);
        memcpy(pCamData, &camData, sizeof(CameraData));

        RenderPassBeginInfo beginInfo = { };
        beginInfo.renderPass = this->m_renderPass;
        beginInfo.framebuffer = this->m_framebuffers[nFace];
        beginInfo.renderArea.extent = { SKYBOX_FACE_SIZE, SKYBOX_FACE_SIZE };
        beginInfo.clearValues = Vector{ clearValue };

        context->BeginRenderPass(beginInfo);

        context->SetViewport(viewport);
        context->SetScissor(scissor);
        context->BindPipeline(this->m_pipeline);
        context->BindDescriptorSets(
            0,
            Vector{ this->m_camSet, this->m_sunSet },
            Vector{ nCamDataOffset, nSunDataOffset }
        );

        context->BindVertexBuffers(Vector{ this->m_cubeVBO });
        context->BindIndexBuffer(this->m_cubeIBO);

        context->DrawIndexed(this->m_nIndexCount);

        context->EndRenderPass();
    }
    context->ImageBarrier(this->m_skybox, EImageLayout::COLOR_ATTACHMENT, EImageLayout::SHADER_READ_ONLY, 6, 0, 0);
    context->GlobalBarrier();
}

/**
* Create sky atmosphere resources
*/
void 
SkyAtmosphere::CreateResources() {
    /* Create cube VBO and IBO (same vertices and indices as in IBL generator) */
    Vector<glm::vec3> vertices = {
        // Back face
        { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, 1.f, -1.f }, { -1.f, 1.f, -1.f },
        // Front face
        { -1.f, -1.f, 1.f }, { 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, { -1.f, 1.f, 1.f  },
        // Left face
        { -1.f, -1.f, -1.f }, { -1.f, -1.f, 1.f }, { -1.f, 1.f, 1.f }, { -1.f, 1.f, -1.f },
        // Right face
        { 1.f, -1.f, -1.f }, { 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f },
        // Bottom face
        { -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, -1.f, 1.f }, { -1.f, -1.f, 1.f },
        // Top Face
        { -1.f, 1.f, -1.f }, { 1.f, 1.f, -1.f }, { 1.f, 1.f, 1.f }, { -1.f, 1.f, 1.f }
    };

    Vector<uint16_t> indices = {
       0, 1, 2, 2, 3, 0, // Back
       4, 5, 6, 6, 7, 4, // Front
       8, 9, 10, 10, 11, 8, // Left
       12, 13, 14, 14, 15, 12, // Right
       16, 17, 18, 18, 19, 16, // Bottom
       20, 21, 22, 22, 23, 20 // Top
    };

    this->m_nIndexCount = indices.size();

    BufferCreateInfo vboInfo = { };
    vboInfo.nSize = vertices.size() * sizeof(glm::vec3);
    vboInfo.pcData = vertices.data();
    vboInfo.usage = EBufferUsage::VERTEX_BUFFER | EBufferUsage::TRANSFER_DST;
    vboInfo.type = EBufferType::VERTEX_BUFFER;
    vboInfo.sharingMode = ESharingMode::EXCLUSIVE;

    BufferCreateInfo iboInfo = { };
    iboInfo.nSize = indices.size() * sizeof(uint16_t);
    iboInfo.pcData = indices.data();
    iboInfo.usage = EBufferUsage::INDEX_BUFFER | EBufferUsage::TRANSFER_DST;
    iboInfo.type = EBufferType::INDEX_BUFFER;
    iboInfo.sharingMode = ESharingMode::EXCLUSIVE;

    this->m_cubeVBO = this->m_device->CreateBuffer(vboInfo);
    this->m_cubeIBO = this->m_device->CreateBuffer(iboInfo);

    /* Create ring buffers */
    uint32_t nSunDataSize = sizeof(SunData);
    uint32_t nCamDataSize = sizeof(CameraData);

    RingBufferCreateInfo sunDataInfo = { };
    sunDataInfo.nAlignment = nSunDataSize;
    sunDataInfo.nFramesInFlight = this->m_nFramesInFlight;
    sunDataInfo.nBufferSize = nSunDataSize * this->m_nFramesInFlight;
    sunDataInfo.usage = EBufferUsage::UNIFORM_BUFFER;

    /* Camera ring buffer: 6 allocations per frame (one per cubemap face) */
    RingBufferCreateInfo camDataInfo = { };
    camDataInfo.nAlignment = nCamDataSize;
    camDataInfo.nFramesInFlight = this->m_nFramesInFlight;
    camDataInfo.nBufferSize = nCamDataSize * 6 * this->m_nFramesInFlight;
    camDataInfo.usage = EBufferUsage::UNIFORM_BUFFER;

    this->m_sunDataBuff = this->m_device->CreateRingBuffer(sunDataInfo);
    this->m_camDataBuff = this->m_device->CreateRingBuffer(camDataInfo);

    /* Create skybox texture */
    TextureCreateInfo skyboxInfo = { };
    skyboxInfo.format = GPUFormat::RGBA16_FLOAT;
    skyboxInfo.nArrayLayers = 6;
    skyboxInfo.nMipLevels = 1;
    skyboxInfo.initialLayout = ETextureLayout::UNDEFINED;
    skyboxInfo.imageType = ETextureDimensions::TYPE_2D;
    skyboxInfo.extent.width = SKYBOX_FACE_SIZE;
    skyboxInfo.extent.height = SKYBOX_FACE_SIZE;
    skyboxInfo.extent.depth = 1.f;
    skyboxInfo.sharingMode = ESharingMode::EXCLUSIVE;
    skyboxInfo.tiling = ETextureTiling::OPTIMAL;
    skyboxInfo.samples = ESampleCount::SAMPLE_1;
    skyboxInfo.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
    skyboxInfo.flags = ETextureFlags::CUBE_COMPATIBLE;

    this->m_skybox = this->m_device->CreateTexture(skyboxInfo);

    /* Create skybox image view (cube) */
    ImageViewCreateInfo viewInfo = { };
    viewInfo.format = GPUFormat::RGBA16_FLOAT;
    viewInfo.image = this->m_skybox;
    viewInfo.viewType = EImageViewType::TYPE_CUBE;
    viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    viewInfo.subresourceRange.nBaseArrayLayer = 0;
    viewInfo.subresourceRange.nLayerCount = 6;
    viewInfo.subresourceRange.nBaseMipLevel = 0;
    viewInfo.subresourceRange.nLevelCount = 1;
    
    this->m_skyboxView = this->m_device->CreateImageView(viewInfo);
} 

/**
* Creates sky atmosphere descriptors
*/
void 
SkyAtmosphere::CreateDescriptors() {
    /* Descriptor set layout bindings */
    DescriptorSetLayoutBinding camBinding = { };
    camBinding.nBinding = 0;
    camBinding.nDescriptorCount = 1;
    camBinding.stageFlags = EShaderStage::VERTEX;
    camBinding.descriptorType = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;
    
    DescriptorSetLayoutBinding sunBinding = { };
    sunBinding.nBinding = 0;
    sunBinding.nDescriptorCount = 1;
    sunBinding.stageFlags = EShaderStage::FRAGMENT;
    sunBinding.descriptorType = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;

    /* Create descriptor set layouts */
    DescriptorSetLayoutCreateInfo camLayoutInfo = { };
    camLayoutInfo.bindings = Vector{ camBinding };
    
    DescriptorSetLayoutCreateInfo sunLayoutInfo = { };
    sunLayoutInfo.bindings = Vector{ sunBinding };
    
    this->m_camSetLayout = this->m_device->CreateDescriptorSetLayout(camLayoutInfo);
    this->m_sunSetLayout = this->m_device->CreateDescriptorSetLayout(sunLayoutInfo);

    /* Descriptor pool sizes */
    DescriptorPoolSize sunSize = { };
    sunSize.type = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;
    sunSize.nDescriptorCount = 1;
   
    DescriptorPoolSize camSize = { };
    camSize.type = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;
    camSize.nDescriptorCount = 1;

    /* Create descriptor pools */
    DescriptorPoolCreateInfo sunPoolInfo = { };
    sunPoolInfo.nMaxSets = 1;
    sunPoolInfo.poolSizes = Vector{ sunSize };

    DescriptorPoolCreateInfo camPoolInfo = { };
    camPoolInfo.nMaxSets = 1;
    camPoolInfo.poolSizes = Vector{ camSize };

    this->m_sunPool = this->m_device->CreateDescriptorPool(sunPoolInfo);
    this->m_camPool = this->m_device->CreateDescriptorPool(camPoolInfo);

    /* Allocate descriptor sets */
    this->m_sunSet = this->m_device->CreateDescriptorSet(this->m_sunPool, this->m_sunSetLayout);
    this->m_camSet = this->m_device->CreateDescriptorSet(this->m_camPool, this->m_camSetLayout);

    /* Write descriptor sets */
    DescriptorBufferInfo sunInfo = { };
    sunInfo.buffer = this->m_sunDataBuff->GetBuffer();
    sunInfo.nRange = this->m_sunDataBuff->GetPerFrameSize();
    sunInfo.nOffset = 0;

    DescriptorBufferInfo camInfo = { };
    camInfo.buffer = this->m_camDataBuff->GetBuffer();
    camInfo.nRange = sizeof(CameraData);
    camInfo.nOffset = 0;

    this->m_sunSet->WriteBuffer(0, 0, sunInfo);
    this->m_camSet->WriteBuffer(0, 0, camInfo);

    this->m_sunSet->UpdateWrites();
    this->m_camSet->UpdateWrites();
}

/**
* Creates the sky atmosphere graphics pipeline
*/
void 
SkyAtmosphere::CreatePipeline() {
    /* Create pipeline layout */
    PipelineLayoutCreateInfo layoutInfo = { };
    layoutInfo.setLayouts = Vector{ this->m_camSetLayout, this->m_sunSetLayout };

    this->m_pipelineLayout = this->m_device->CreatePipelineLayout(layoutInfo);
    
    VertexInputBinding binding = { };
    binding.nBinding = 0;
    binding.nStride = sizeof(glm::vec3);

    VertexInputAttribute attrib = { };
    attrib.format = GPUFormat::RGB32_FLOAT;
    attrib.nBinding = 0;
    attrib.nLocation = 0;
    attrib.nOffset = 0;

    ColorBlendAttachment colorBlend = { };
    colorBlend.bBlendEnable = false;
    colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;

    ColorBlendState blendState = { };
    blendState.attachments = Vector{ colorBlend };

    RasterizationState raster = { };
    raster.cullMode = ECullMode::NONE;
    raster.frontFace = EFrontFace::CLOCKWISE;
    raster.polygonMode = EPolygonMode::FILL;

    Ref<Shader> vertexShader = Shader::CreateShared();
    Ref<Shader> pixelShader = Shader::CreateShared();

    vertexShader->LoadFromGLSL("SkyAtmosphere.vert", EShaderStage::VERTEX);
    pixelShader->LoadFromGLSL("SkyAtmosphere.frag", EShaderStage::FRAGMENT);

    /* Create render pass */
    AttachmentDescription colorAttachment = { };
    colorAttachment.format = GPUFormat::RGBA16_FLOAT;
    colorAttachment.initialLayout = EImageLayout::UNDEFINED;
    colorAttachment.finalLayout = EImageLayout::COLOR_ATTACHMENT;
    colorAttachment.loadOp = EAttachmentLoadOp::CLEAR;
    colorAttachment.storeOp = EAttachmentStoreOp::STORE;
    colorAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
    colorAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
    colorAttachment.sampleCount = ESampleCount::SAMPLE_1;

    AttachmentReference colorAttachmentRef = { };
    colorAttachmentRef.layout = EImageLayout::COLOR_ATTACHMENT;
    colorAttachmentRef.nAttachment = 0;

    SubpassDescription subpass = { };
    subpass.colorAttachments = Vector{ colorAttachmentRef };
    subpass.bHasDepthStencil = false;
    
    RenderPassCreateInfo rpInfo = { };
    rpInfo.subpasses = Vector{ subpass };
    rpInfo.attachments = Vector{ colorAttachment };

    this->m_renderPass = this->m_device->CreateRenderPass(rpInfo);

    /* Create graphics pipeline */
    GraphicsPipelineCreateInfo pipelineInfo = { };
    pipelineInfo.vertexBindings = Vector{ binding };
    pipelineInfo.vertexAttributes = Vector{ attrib };
    pipelineInfo.colorFormats = Vector{ GPUFormat::RGBA16_FLOAT };
    pipelineInfo.multisampleState.nSampleCount = 1;
    pipelineInfo.colorBlendState = blendState;
    pipelineInfo.nSubpass = 0;
    pipelineInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;
    pipelineInfo.rasterizationState = raster;
    pipelineInfo.shaders = Vector{ vertexShader, pixelShader };
    pipelineInfo.renderPass = this->m_renderPass;
    pipelineInfo.pipelineLayout = this->m_pipelineLayout;
    
    this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);

    /* Create framebuffers */
    this->m_imageViews.resize(6);
    this->m_framebuffers.resize(6);

    for (uint32_t nFace = 0; nFace < 6; nFace++) {
        /* Create image view for the face */
        ImageViewCreateInfo viewInfo = { };
        viewInfo.format = GPUFormat::RGBA16_FLOAT;
        viewInfo.image = this->m_skybox;
        viewInfo.viewType = EImageViewType::TYPE_2D;
        viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
        viewInfo.subresourceRange.nBaseArrayLayer = nFace;
        viewInfo.subresourceRange.nLayerCount = 1;
        viewInfo.subresourceRange.nBaseMipLevel = 0;
        viewInfo.subresourceRange.nLevelCount = 1;

        this->m_imageViews[nFace] = this->m_device->CreateImageView(viewInfo);

        /* Create a framebuffer for the face */
        FramebufferCreateInfo fbInfo = { };
        fbInfo.nLayers = 1;
        fbInfo.nWidth = SKYBOX_FACE_SIZE;
        fbInfo.nHeight = SKYBOX_FACE_SIZE;
        fbInfo.renderPass = this->m_renderPass;
        fbInfo.attachments = { this->m_imageViews[nFace] };

        this->m_framebuffers[nFace] = this->m_device->CreateFramebuffer(fbInfo);
    }
}

/**
* Sets the sun direction
*
* @param sunDir Sun direction
*/
void 
SkyAtmosphere::SetSunDirection(const glm::vec3& sunDir) {
    this->m_sunDir = sunDir;
}

/**
* Set camera view projection
* 
* @param view Camera view
* @param proj Camera projection
*/
void 
SkyAtmosphere::SetViewProjection(const glm::mat4& view, const glm::mat4& proj) {
    this->m_view = view;
    this->m_proj = proj;
}