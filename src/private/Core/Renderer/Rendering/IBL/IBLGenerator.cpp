#include "Core/Renderer/Rendering/IBL/IBLGenerator.h"

void 
IBLGenerator::Init(
    Ref<Device> device, 
    Ref<GPUTexture> skybox,
    Ref<ImageView> skyboxView,
    Ref<Sampler> sampler,
    Ref<GPUBuffer> sqVBO,
    Ref<GPUBuffer> sqIBO,
    uint32_t nFramesInFlight
) {
    this->m_device = device;
    this->m_nFramesInFlight = nFramesInFlight;
    this->m_skybox = skybox;
    this->m_skyboxView = skyboxView;
    this->m_sampler = sampler;
    this->m_sqVBO = sqVBO;
    this->m_sqIBO = sqIBO;

    this->CreateDescriptors();
    this->CreateRenderPasses();
    this->CreatePipelines();
    this->CreateResources();

    /* Write descriptor sets (once at init, not per frame) */
    DescriptorImageInfo skyboxInfo = { };
    skyboxInfo.texture = this->m_skybox;
    skyboxInfo.imageView = this->m_skyboxView;
    skyboxInfo.sampler = this->m_sampler;

    this->m_irradianceSet->WriteTexture(0, 0, skyboxInfo);
    this->m_prefilterSet->WriteTexture(0, 0, skyboxInfo);

    this->m_irradianceSet->UpdateWrites();
    this->m_prefilterSet->UpdateWrites();

    /* Write viewProj descriptor */
    DescriptorBufferInfo viewProjInfo = { };
    viewProjInfo.buffer = this->m_viewProjBuffer->GetBuffer();
    viewProjInfo.nOffset = 0;
    viewProjInfo.nRange = sizeof(ViewProjection);

    this->m_viewProjSet->WriteBuffer(0, 0, viewProjInfo);
    this->m_viewProjSet->UpdateWrites();
}

void
IBLGenerator::Generate(Ref<GraphicsContext> context, uint32_t nFrameIdx) {
    this->m_viewProjBuffer->Reset(nFrameIdx);

    /* Projection matrices for each cubemap face */
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.f, .1f, 10.f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0.f), glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)), // +X
        glm::lookAt(glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)), // -X
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)), // +Y
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)), // -Y
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)), // +Z
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f)) // -Z
    };

    ClearValue clearValue{ { 0.f, 0.f, 0.f, 1.f } };

    context->ImageBarrier(this->m_irradiance, EImageLayout::UNDEFINED, EImageLayout::COLOR_ATTACHMENT, 6, 0);

    for (uint32_t nFace = 0; nFace < 6; nFace++) {        
        RenderPassBeginInfo beginInfo = { };
        beginInfo.renderPass = this->m_irradianceRP;
        beginInfo.framebuffer = this->m_irradianceFBs[nFace];
        beginInfo.renderArea.extent = { IRRADIANCE_SIZE, IRRADIANCE_SIZE };
        beginInfo.clearValues = Vector{ clearValue };

        context->BeginRenderPass(beginInfo);
        
        Viewport viewport = { };
        viewport.width = static_cast<float>(IRRADIANCE_SIZE);
        viewport.height = -static_cast<float>(IRRADIANCE_SIZE);
        viewport.x = 0.f;
        viewport.y = static_cast<float>(IRRADIANCE_SIZE);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        Rect2D scissor = { };
        scissor.extent = { IRRADIANCE_SIZE, IRRADIANCE_SIZE };
        scissor.offset = { 0, 0 };

        context->SetViewport(viewport);
        context->SetScissor(scissor);

        context->BindPipeline(this->m_irradiancePipeline);
        context->BindDescriptorSets(0, Vector{ this->m_irradianceSet });

        struct {
            glm::mat4 View;
            glm::mat4 Projection;
        } pushData;

        pushData.View = captureViews[nFace];
        pushData.Projection = captureProjection;

        context->PushConstants(this->m_irradianceLayout, EShaderStage::VERTEX, 0, sizeof(pushData), &pushData);

        context->BindVertexBuffers(Vector{ this->m_cubeVBO });
        context->BindIndexBuffer(this->m_cubeIBO);

        context->DrawIndexed(this->m_nIndexCount);

        context->EndRenderPass();
    }

    context->ImageBarrier(
        this->m_irradiance,
        EImageLayout::COLOR_ATTACHMENT,
        EImageLayout::SHADER_READ_ONLY,
        6,
        0
    );

    for (uint32_t nMip = 0; nMip < MIP_LEVELS; nMip++) {
        uint32_t nMipSize = PREFILTER_SIZE >> nMip;
        float roughness = static_cast<float>(nMip) / static_cast<float>(MIP_LEVELS - 1);

        context->ImageBarrier(this->m_prefilter, EImageLayout::UNDEFINED, EImageLayout::COLOR_ATTACHMENT, 6, nMip);

        for (uint32_t nFace = 0; nFace < 6; nFace++) {
            RenderPassBeginInfo beginInfo = { };
            beginInfo.renderPass = this->m_prefilterRP;
            beginInfo.framebuffer = this->m_prefilterFBs[nMip][nFace];
            beginInfo.renderArea.extent = { nMipSize, nMipSize };
            beginInfo.clearValues = Vector{ clearValue };
            
            context->BeginRenderPass(beginInfo);

            context->BindPipeline(this->m_prefilterPipeline);

            ViewProjection viewProj = {
               captureViews[nFace],
               captureProjection
            };

            uint32_t nViewProjectionOffset = 0;
            void* pViewProjection = this->m_viewProjBuffer->Allocate(sizeof(viewProj), nViewProjectionOffset);
            memcpy(pViewProjection, &viewProj, sizeof(viewProj));

            context->BindDescriptorSets(
                0, 
                Vector{ this->m_prefilterSet }
            );

            context->BindDescriptorSets(
                1,
                Vector{ this->m_viewProjSet },
                Vector{ nViewProjectionOffset }
            );
            
            struct {
                float roughness;
                uint32_t nMipLevel;
            } pushData;

            pushData.roughness = roughness;
            pushData.nMipLevel = nMip;

            context->PushConstants(
                this->m_prefilterLayout, 
                EShaderStage::VERTEX | EShaderStage::FRAGMENT,
                0, 
                sizeof(pushData), 
                &pushData
            );

            Viewport viewport = { };
            viewport.width = static_cast<float>(nMipSize);
            viewport.height = -static_cast<float>(nMipSize);
            viewport.x = 0.f;
            viewport.y = static_cast<float>(nMipSize);
            viewport.minDepth = 0.f;
            viewport.maxDepth = 1.f;

            Rect2D scissor = { };
            scissor.extent = { nMipSize, nMipSize };
            scissor.offset = { 0, 0 };

            context->SetViewport(viewport);
            context->SetScissor(scissor);

            context->BindVertexBuffers(Vector{ this->m_cubeVBO });
            context->BindIndexBuffer(this->m_cubeIBO);

            context->DrawIndexed(this->m_nIndexCount);
            context->EndRenderPass();
        }

        context->ImageBarrier(
            this->m_prefilter,
            EImageLayout::COLOR_ATTACHMENT,
            EImageLayout::SHADER_READ_ONLY,
            6,
            nMip
        );
    }

    context->ImageBarrier(this->m_brdf, EImageLayout::UNDEFINED, EImageLayout::COLOR_ATTACHMENT);

    {
        RenderPassBeginInfo beginInfo = { };
        beginInfo.framebuffer = this->m_brdfFB;
        beginInfo.renderPass = this->m_brdfRP;
        beginInfo.clearValues = Vector{ clearValue };
        beginInfo.renderArea.extent = { BRDF_SIZE, BRDF_SIZE };
        
        context->BeginRenderPass(beginInfo);

        Viewport viewport = { };
        viewport.width = static_cast<float>(BRDF_SIZE);
        viewport.height = static_cast<float>(BRDF_SIZE);
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        Rect2D scissor = { };
        scissor.extent = { BRDF_SIZE, BRDF_SIZE };
        scissor.offset = { 0, 0 };

        context->SetViewport(viewport);
        context->SetScissor(scissor);

        context->BindVertexBuffers(Vector{ this->m_sqVBO });
        context->BindIndexBuffer(this->m_sqIBO);
        context->BindPipeline(this->m_brdfPipeline);

        context->DrawIndexed(6);

        context->EndRenderPass();
    }

    context->ImageBarrier(
        this->m_brdf,
        EImageLayout::COLOR_ATTACHMENT,
        EImageLayout::SHADER_READ_ONLY
    );
}

void 
IBLGenerator::CreateDescriptors() {
    DescriptorSetLayoutBinding samplerBinding = { };
    samplerBinding.nBinding = 0;
    samplerBinding.descriptorType = EDescriptorType::COMBINED_IMAGE_SAMPLER;
    samplerBinding.nDescriptorCount = 1;
    samplerBinding.stageFlags = EShaderStage::FRAGMENT;

    DescriptorSetLayoutCreateInfo irradianceLayoutInfo = { };
    irradianceLayoutInfo.bindings = Vector{ samplerBinding };

    this->m_irradianceSetLayout = this->m_device->CreateDescriptorSetLayout(irradianceLayoutInfo);
    
    DescriptorSetLayoutCreateInfo prefilterLayoutInfo = { };
    prefilterLayoutInfo.bindings = Vector{ samplerBinding };

    this->m_prefilterSetLayout = this->m_device->CreateDescriptorSetLayout(prefilterLayoutInfo);

    DescriptorPoolSize poolSize = { };
    poolSize.nDescriptorCount = 1;
    poolSize.type = EDescriptorType::COMBINED_IMAGE_SAMPLER;

    DescriptorPoolCreateInfo poolInfo = { };
    poolInfo.nMaxSets = 1;
    poolInfo.poolSizes = Vector{ poolSize };

    this->m_irradiancePool = this->m_device->CreateDescriptorPool(poolInfo);
    this->m_prefilterPool = this->m_device->CreateDescriptorPool(poolInfo);

    this->m_irradianceSet = this->m_device->CreateDescriptorSet(this->m_irradiancePool, this->m_irradianceSetLayout);
    this->m_prefilterSet = this->m_device->CreateDescriptorSet(this->m_prefilterPool, this->m_prefilterSetLayout);

    /* View projection descriptor set */
    DescriptorSetLayoutBinding viewProjBinding = { };
    viewProjBinding.descriptorType = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;
    viewProjBinding.nBinding = 0;
    viewProjBinding.nDescriptorCount = 1;
    viewProjBinding.stageFlags = EShaderStage::VERTEX;

    DescriptorSetLayoutCreateInfo vpLayoutInfo = { };
    vpLayoutInfo.bindings = Vector{ viewProjBinding };
    
    this->m_viewProjSetLayout = this->m_device->CreateDescriptorSetLayout(vpLayoutInfo);

    DescriptorPoolSize vpPoolSize = { };
    vpPoolSize.nDescriptorCount = 1;
    vpPoolSize.type = EDescriptorType::UNIFORM_BUFFER_DYNAMIC;

    DescriptorPoolCreateInfo vpPoolInfo = { };
    vpPoolInfo.nMaxSets = 1;
    vpPoolInfo.poolSizes = Vector{ vpPoolSize };

    this->m_viewProjPool = this->m_device->CreateDescriptorPool(vpPoolInfo);

    this->m_viewProjSet = this->m_device->CreateDescriptorSet(this->m_viewProjPool, this->m_viewProjSetLayout);
}

void 
IBLGenerator::CreateRenderPasses() {
    /* Irradiance render pass */
    AttachmentDescription irradianceAttachment = { };
    irradianceAttachment.format = CUBEMAP_FORMAT;
    irradianceAttachment.sampleCount = ESampleCount::SAMPLE_1;
    irradianceAttachment.loadOp = EAttachmentLoadOp::CLEAR;
    irradianceAttachment.storeOp = EAttachmentStoreOp::STORE;
    irradianceAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
    irradianceAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
    irradianceAttachment.initialLayout = EImageLayout::UNDEFINED;
    irradianceAttachment.finalLayout = EImageLayout::COLOR_ATTACHMENT;

    AttachmentReference irradianceRef = { };
    irradianceRef.nAttachment = 0;
    irradianceRef.layout = EImageLayout::COLOR_ATTACHMENT;
    
    SubpassDescription irradianceSubpass = { };
    irradianceSubpass.colorAttachments = Vector{ irradianceRef };

    RenderPassCreateInfo irradianceRPInfo = { };
    irradianceRPInfo.attachments = Vector{ irradianceAttachment };
    irradianceRPInfo.subpasses = Vector{ irradianceSubpass };

    this->m_irradianceRP = this->m_device->CreateRenderPass(irradianceRPInfo);

    /* Prefilter render pass */
    AttachmentDescription prefilterAttachment = { };
    prefilterAttachment.format = CUBEMAP_FORMAT;
    prefilterAttachment.sampleCount = ESampleCount::SAMPLE_1;
    prefilterAttachment.loadOp = EAttachmentLoadOp::CLEAR;
    prefilterAttachment.storeOp = EAttachmentStoreOp::STORE;
    prefilterAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
    prefilterAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
    prefilterAttachment.initialLayout = EImageLayout::UNDEFINED;
    prefilterAttachment.finalLayout = EImageLayout::COLOR_ATTACHMENT;

    AttachmentReference prefilterRef = { };
    prefilterRef.nAttachment = 0;
    prefilterRef.layout = EImageLayout::COLOR_ATTACHMENT;

    SubpassDescription prefilterSubpass = { };
    prefilterSubpass.colorAttachments = Vector{ prefilterRef };

    RenderPassCreateInfo prefilterRPInfo = { };
    prefilterRPInfo.attachments = Vector{ prefilterAttachment };
    prefilterRPInfo.subpasses = Vector{ prefilterSubpass };

    this->m_prefilterRP = this->m_device->CreateRenderPass(prefilterRPInfo);

    /* BRDF render pass */
    AttachmentDescription brdfAttachment = { };
    brdfAttachment.format = BRDF_FORMAT;
    brdfAttachment.sampleCount = ESampleCount::SAMPLE_1;
    brdfAttachment.loadOp = EAttachmentLoadOp::CLEAR;
    brdfAttachment.storeOp = EAttachmentStoreOp::STORE;
    brdfAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
    brdfAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
    brdfAttachment.initialLayout = EImageLayout::UNDEFINED;
    brdfAttachment.finalLayout = EImageLayout::COLOR_ATTACHMENT;

    AttachmentReference brdfRef = { };
    brdfRef.nAttachment = 0;
    brdfRef.layout = EImageLayout::COLOR_ATTACHMENT;

    SubpassDescription brdfSubpass = { };
    brdfSubpass.colorAttachments = Vector{ brdfRef };

    RenderPassCreateInfo brdfRPInfo = { };
    brdfRPInfo.attachments = Vector{ brdfAttachment };
    brdfRPInfo.subpasses = Vector{ brdfSubpass };

    this->m_brdfRP = this->m_device->CreateRenderPass(brdfRPInfo);
}

void
IBLGenerator::CreatePipelines() {
    RasterizationState iblRaster = { };
    iblRaster.cullMode = ECullMode::NONE;
    iblRaster.frontFace = EFrontFace::COUNTER_CLOCKWISE;
    iblRaster.polygonMode = EPolygonMode::FILL;
    iblRaster.lineWidth = 1.f;

    MultisampleState iblMS = { };
    iblMS.nSampleCount = 1;

    ColorBlendAttachment colorBlend = { };
    colorBlend.bBlendEnable = false;
    colorBlend.bWriteR = colorBlend.bWriteG = colorBlend.bWriteB = colorBlend.bWriteA = true;

    ColorBlendState blendState = { };
    blendState.bLogicOpEnable = false;
    blendState.attachments = { colorBlend };

    DepthStencilState dsState = { };
    dsState.bDepthTestEnable = false;
    dsState.bDepthWriteEnable = false;
    dsState.bStencilTestEnable = false;

    PushConstantRange pushRange = { };

    struct ScreenQuadVertex {
        glm::vec3 position;
        glm::vec2 texCoord;
    };

    /* Irradiance pipeline */
    VertexInputBinding irradianceBindingDesc(1);
    irradianceBindingDesc.nBinding = 0;
    irradianceBindingDesc.nStride = sizeof(glm::vec3);
    irradianceBindingDesc.bPerInstance = false;

    VertexInputAttribute irradianceAttrib = { };
    irradianceAttrib.nBinding = 0;
    irradianceAttrib.nOffset = 0;
    irradianceAttrib.format = GPUFormat::RGB32_FLOAT;
    irradianceAttrib.nLocation = 0;

    Ref<Shader> irradianceVS = Shader::CreateShared();
    Ref<Shader> irradiancePS = Shader::CreateShared();

    irradianceVS->LoadFromGLSL("IrradianceConvolution.vert", EShaderStage::VERTEX);
    irradiancePS->LoadFromGLSL("IrradianceConvolution.frag", EShaderStage::FRAGMENT);

    pushRange.stage = EShaderStage::VERTEX;
    pushRange.nSize = sizeof(glm::mat4) * 2;

    PipelineLayoutCreateInfo irradianceLayoutInfo = { };
    irradianceLayoutInfo.setLayouts = { this->m_irradianceSetLayout };
    irradianceLayoutInfo.pushConstantRanges = { pushRange };

    this->m_irradianceLayout = this->m_device->CreatePipelineLayout(irradianceLayoutInfo);

    GraphicsPipelineCreateInfo irradiancePipelineInfo = { };
    irradiancePipelineInfo.colorBlendState = blendState;
    irradiancePipelineInfo.vertexBindings = { irradianceBindingDesc };
    irradiancePipelineInfo.vertexAttributes = { irradianceAttrib };
    irradiancePipelineInfo.nSubpass = 0;
    irradiancePipelineInfo.colorFormats = { CUBEMAP_FORMAT };
    irradiancePipelineInfo.rasterizationState = iblRaster;
    irradiancePipelineInfo.multisampleState = iblMS;
    irradiancePipelineInfo.shaders = { irradianceVS, irradiancePS };
    irradiancePipelineInfo.pipelineLayout = this->m_irradianceLayout;
    irradiancePipelineInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;
    irradiancePipelineInfo.depthStencilState = dsState;
    irradiancePipelineInfo.renderPass = this->m_irradianceRP;

    this->m_irradiancePipeline = this->m_device->CreateGraphicsPipeline(irradiancePipelineInfo);

    /* Prefilter pipeline */
    VertexInputBinding prefilterBindingDesc(1);
    prefilterBindingDesc.nBinding = 0;
    prefilterBindingDesc.nStride = sizeof(glm::vec3);
    prefilterBindingDesc.bPerInstance = false;

    VertexInputAttribute prefilterAttrib = { };
    prefilterAttrib.nBinding = 0;
    prefilterAttrib.nOffset = 0;
    prefilterAttrib.format = GPUFormat::RGB32_FLOAT;
    prefilterAttrib.nLocation = 0;

    Ref<Shader> prefilterVS = Shader::CreateShared();
    Ref<Shader> prefilterPS = Shader::CreateShared();

    prefilterVS->LoadFromGLSL("PrefilterEnvMap.vert", EShaderStage::VERTEX);
    prefilterPS->LoadFromGLSL("PrefilterEnvMap.frag", EShaderStage::FRAGMENT);

    pushRange.stage = EShaderStage::VERTEX | EShaderStage::FRAGMENT;
    pushRange.nSize = sizeof(uint32_t) * 2;

    PipelineLayoutCreateInfo prefilterLayoutInfo = { };
    prefilterLayoutInfo.setLayouts = { this->m_prefilterSetLayout, this->m_viewProjSetLayout };
    prefilterLayoutInfo.pushConstantRanges = { pushRange };

    this->m_prefilterLayout = this->m_device->CreatePipelineLayout(prefilterLayoutInfo);

    GraphicsPipelineCreateInfo prefilterPipelineInfo = { };
    prefilterPipelineInfo.colorBlendState = blendState;
    prefilterPipelineInfo.vertexBindings = { prefilterBindingDesc };
    prefilterPipelineInfo.vertexAttributes = { prefilterAttrib };
    prefilterPipelineInfo.nSubpass = 0;
    prefilterPipelineInfo.colorFormats = { CUBEMAP_FORMAT };
    prefilterPipelineInfo.rasterizationState = iblRaster;
    prefilterPipelineInfo.multisampleState = iblMS;
    prefilterPipelineInfo.shaders = { prefilterVS, prefilterPS };
    prefilterPipelineInfo.pipelineLayout = this->m_prefilterLayout;
    prefilterPipelineInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;
    prefilterPipelineInfo.depthStencilState = dsState;
    prefilterPipelineInfo.renderPass = this->m_prefilterRP;

    this->m_prefilterPipeline = this->m_device->CreateGraphicsPipeline(prefilterPipelineInfo);

    /* BRDF pipeline */
    VertexInputBinding brdfBindingDesc(2);
    brdfBindingDesc.nBinding = 0;
    brdfBindingDesc.nStride = sizeof(ScreenQuadVertex);
    brdfBindingDesc.bPerInstance = false;

    Vector<VertexInputAttribute> brdfAttribs(2);
    brdfAttribs[0].nBinding = 0;
    brdfAttribs[0].nOffset = offsetof(ScreenQuadVertex, position);
    brdfAttribs[0].format = GPUFormat::RGB32_FLOAT;
    brdfAttribs[0].nLocation = 0;

    brdfAttribs[1].nBinding = 0;
    brdfAttribs[1].nOffset = offsetof(ScreenQuadVertex, texCoord);
    brdfAttribs[1].format = GPUFormat::RG32_FLOAT;
    brdfAttribs[1].nLocation = 1;

    Ref<Shader> brdfVS = Shader::CreateShared();
    Ref<Shader> brdfPS = Shader::CreateShared();

    brdfVS->LoadFromGLSL("BRDFIntegration.vert", EShaderStage::VERTEX);
    brdfPS->LoadFromGLSL("BRDFIntegration.frag", EShaderStage::FRAGMENT);

    PipelineLayoutCreateInfo brdfLayoutInfo = { };

    this->m_brdfLayout = this->m_device->CreatePipelineLayout(brdfLayoutInfo);

    GraphicsPipelineCreateInfo brdfPipelineInfo = { };
    brdfPipelineInfo.colorBlendState = blendState;
    brdfPipelineInfo.vertexBindings = { brdfBindingDesc };
    brdfPipelineInfo.vertexAttributes = brdfAttribs;
    brdfPipelineInfo.nSubpass = 0;
    brdfPipelineInfo.colorFormats = { BRDF_FORMAT };
    brdfPipelineInfo.rasterizationState = iblRaster;
    brdfPipelineInfo.multisampleState = iblMS;
    brdfPipelineInfo.shaders = { brdfVS, brdfPS };
    brdfPipelineInfo.pipelineLayout = this->m_brdfLayout;
    brdfPipelineInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;
    brdfPipelineInfo.depthStencilState = dsState;
    brdfPipelineInfo.renderPass = this->m_brdfRP;

    this->m_brdfPipeline = this->m_device->CreateGraphicsPipeline(brdfPipelineInfo);
}

void
IBLGenerator::CreateResources() {
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

    BufferCreateInfo vboInfo = { };
    vboInfo.nSize = vertices.size() * sizeof(glm::vec3);
    vboInfo.type = EBufferType::VERTEX_BUFFER;
    vboInfo.pcData = vertices.data();
    vboInfo.sharingMode = ESharingMode::EXCLUSIVE;
    vboInfo.usage = EBufferUsage::VERTEX_BUFFER | EBufferUsage::TRANSFER_DST;
    
    BufferCreateInfo iboInfo = { };
    iboInfo.nSize = indices.size() * sizeof(uint16_t);
    iboInfo.type = EBufferType::INDEX_BUFFER;
    iboInfo.pcData = indices.data();
    iboInfo.sharingMode = ESharingMode::EXCLUSIVE;
    iboInfo.usage = EBufferUsage::INDEX_BUFFER | EBufferUsage::TRANSFER_DST;

    this->m_cubeVBO = this->m_device->CreateBuffer(vboInfo);
    this->m_cubeIBO = this->m_device->CreateBuffer(iboInfo);

    this->m_nIndexCount = indices.size();

    /* Create view projection ring buffer */
    uint32_t nViewProjectionSize = sizeof(ViewProjection);

    RingBufferCreateInfo viewProjInfo = { };
    viewProjInfo.nAlignment = nViewProjectionSize;
    viewProjInfo.nFramesInFlight = this->m_nFramesInFlight;
    viewProjInfo.nBufferSize = nViewProjectionSize * 36 * this->m_nFramesInFlight;
    viewProjInfo.usage = EBufferUsage::UNIFORM_BUFFER;

    this->m_viewProjBuffer = this->m_device->CreateRingBuffer(viewProjInfo);

    TextureCreateInfo cubemapInfo = { };
    cubemapInfo.format = CUBEMAP_FORMAT;
    cubemapInfo.imageType = ETextureDimensions::TYPE_2D;
    cubemapInfo.extent.width = IRRADIANCE_SIZE;
    cubemapInfo.extent.height = IRRADIANCE_SIZE;
    cubemapInfo.extent.depth = 1;
    cubemapInfo.nMipLevels = 1;
    cubemapInfo.nArrayLayers = 6;
    cubemapInfo.flags = ETextureFlags::CUBE_COMPATIBLE;
    cubemapInfo.tiling = ETextureTiling::OPTIMAL;
    cubemapInfo.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
    cubemapInfo.sharingMode = ESharingMode::EXCLUSIVE;
    cubemapInfo.initialLayout = ETextureLayout::UNDEFINED;
    cubemapInfo.samples = ESampleCount::SAMPLE_1;
    
    this->m_irradiance = this->m_device->CreateTexture(cubemapInfo);

    this->m_irradianceFBs.resize(6);
    this->m_irradianceFaceViews.resize(6);
    for (uint32_t nFace = 0; nFace < 6; nFace++) {
        ImageViewCreateInfo viewInfo = { };
        viewInfo.image = this->m_irradiance;
        viewInfo.viewType = EImageViewType::TYPE_2D;
        viewInfo.format = CUBEMAP_FORMAT;
        viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
        viewInfo.subresourceRange.nBaseMipLevel = 0;
        viewInfo.subresourceRange.nLevelCount = 1;
        viewInfo.subresourceRange.nBaseArrayLayer = nFace;
        viewInfo.subresourceRange.nLayerCount = 1;
        
        this->m_irradianceFaceViews[nFace] = this->m_device->CreateImageView(viewInfo);

        FramebufferCreateInfo fbInfo = { };
        fbInfo.renderPass = this->m_irradianceRP;
        fbInfo.attachments = Vector{ this->m_irradianceFaceViews[nFace] };
        fbInfo.nLayers = 1;
        fbInfo.nWidth = IRRADIANCE_SIZE;
        fbInfo.nHeight = IRRADIANCE_SIZE;

        this->m_irradianceFBs[nFace] = this->m_device->CreateFramebuffer(fbInfo);
    }

    /* Irradiance cubemap view (for sampling) */
    ImageViewCreateInfo cubemapViewInfo = { };
    cubemapViewInfo.format = CUBEMAP_FORMAT;
    cubemapViewInfo.image = this->m_irradiance;
    cubemapViewInfo.viewType = EImageViewType::TYPE_CUBE;
    cubemapViewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    cubemapViewInfo.subresourceRange.nBaseArrayLayer = 0;
    cubemapViewInfo.subresourceRange.nLayerCount = 6;
    cubemapViewInfo.subresourceRange.nBaseMipLevel = 0;
    cubemapViewInfo.subresourceRange.nLevelCount = 1;

    this->m_irradianceView = this->m_device->CreateImageView(cubemapViewInfo);

    cubemapInfo.extent.width = PREFILTER_SIZE;
    cubemapInfo.extent.height = PREFILTER_SIZE;
    cubemapInfo.nMipLevels = MIP_LEVELS;

    this->m_prefilter = this->m_device->CreateTexture(cubemapInfo);

    this->m_prefilterFBs.resize(MIP_LEVELS);
    this->m_prefilterFaceViews.resize(MIP_LEVELS);
    for (uint32_t nMip = 0; nMip < MIP_LEVELS; nMip++) {
        uint32_t nMipSize = PREFILTER_SIZE >> nMip;

        this->m_prefilterFBs[nMip].resize(6);
        this->m_prefilterFaceViews[nMip].resize(6);

        for (uint32_t nFace = 0; nFace < 6; nFace++) {
            ImageViewCreateInfo viewInfo = { };
            viewInfo.image = this->m_prefilter;
            viewInfo.viewType = EImageViewType::TYPE_2D;
            viewInfo.format = CUBEMAP_FORMAT;
            viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
            viewInfo.subresourceRange.nBaseArrayLayer = nFace;
            viewInfo.subresourceRange.nLayerCount = 1;
            viewInfo.subresourceRange.nBaseMipLevel = nMip;
            viewInfo.subresourceRange.nLevelCount = 1;

            this->m_prefilterFaceViews[nMip][nFace] = this->m_device->CreateImageView(viewInfo);

            FramebufferCreateInfo fbInfo = { };
            fbInfo.renderPass = this->m_prefilterRP;
            fbInfo.nWidth = nMipSize;
            fbInfo.nHeight = nMipSize;
            fbInfo.attachments = Vector{ this->m_prefilterFaceViews[nMip][nFace] };
            fbInfo.nLayers = 1;
            
            this->m_prefilterFBs[nMip][nFace] = this->m_device->CreateFramebuffer(fbInfo);
        }
    }

    /* Prefilter cubemap view (for sampling) */
    cubemapViewInfo.image = this->m_prefilter;
    cubemapViewInfo.subresourceRange.nLevelCount = MIP_LEVELS;

    this->m_prefilterView = this->m_device->CreateImageView(cubemapViewInfo);

    TextureCreateInfo brdfTexInfo = { };
    brdfTexInfo.format = BRDF_FORMAT;
    brdfTexInfo.imageType = ETextureDimensions::TYPE_2D;
    brdfTexInfo.samples = ESampleCount::SAMPLE_1;
    brdfTexInfo.nMipLevels = 1;
    brdfTexInfo.nArrayLayers = 1;
    brdfTexInfo.extent = { BRDF_SIZE, BRDF_SIZE, 1 };
    brdfTexInfo.sharingMode = ESharingMode::EXCLUSIVE;
    brdfTexInfo.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
    brdfTexInfo.initialLayout = ETextureLayout::UNDEFINED;
    brdfTexInfo.tiling = ETextureTiling::OPTIMAL;
    
    this->m_brdf = this->m_device->CreateTexture(brdfTexInfo);

    ImageViewCreateInfo brdfFaceViewInfo = { };
    brdfFaceViewInfo.format = BRDF_FORMAT;
    brdfFaceViewInfo.image = this->m_brdf;
    brdfFaceViewInfo.viewType = EImageViewType::TYPE_2D;
    brdfFaceViewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    brdfFaceViewInfo.subresourceRange.nBaseArrayLayer = 0;
    brdfFaceViewInfo.subresourceRange.nLayerCount = 1;
    brdfFaceViewInfo.subresourceRange.nBaseMipLevel = 0;
    brdfFaceViewInfo.subresourceRange.nLevelCount = 1;

    this->m_brdfFaceView = this->m_device->CreateImageView(brdfFaceViewInfo);

    FramebufferCreateInfo brdfFBInfo = { };
    brdfFBInfo.nWidth = BRDF_SIZE;
    brdfFBInfo.nHeight = BRDF_SIZE;
    brdfFBInfo.renderPass = this->m_brdfRP;
    brdfFBInfo.nLayers = 1;
    brdfFBInfo.attachments = Vector{ this->m_brdfFaceView };

    this->m_brdfFB = this->m_device->CreateFramebuffer(brdfFBInfo);

    /* BRDF sampling view */
    ImageViewCreateInfo brdfViewInfo = { };
    brdfViewInfo.format = BRDF_FORMAT;
    brdfViewInfo.image = this->m_brdf;
    brdfViewInfo.viewType = EImageViewType::TYPE_2D;
    brdfViewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    brdfViewInfo.subresourceRange.nBaseArrayLayer = 0;
    brdfViewInfo.subresourceRange.nLayerCount = 1;
    brdfViewInfo.subresourceRange.nBaseMipLevel = 0;
    brdfViewInfo.subresourceRange.nLevelCount = 1;

    this->m_brdfView = this->m_device->CreateImageView(brdfViewInfo);
}