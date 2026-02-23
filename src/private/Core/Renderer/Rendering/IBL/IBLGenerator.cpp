#include "Core/Renderer/Rendering/IBL/IBLGenerator.h"

void 
IBLGenerator::Init(
    Ref<Device> device, 
    Ref<GPUTexture> skybox,
    Ref<ImageView> skyboxView,
    Ref<Sampler> sampler,
    Ref<GPUBuffer> sqVBO,
    Ref<GPUBuffer> sqIBO
) {
    this->m_device = device;
    this->m_skybox = skybox;
    this->m_skyboxView = skyboxView;
    this->m_sampler = sampler;
    this->m_sqVBO = sqVBO;
    this->m_sqIBO = sqIBO;

    this->CreateDescriptors();
    this->CreateRenderPasses();
    this->CreatePipelines();
    this->CreateResources();
}

void
IBLGenerator::Generate(Ref<GraphicsContext> context) {
    uint32_t nIrradianceSize = 64;
    uint32_t nPrefilterSize = 128;
    uint32_t nBRDFSize = 512;

    /*
        5 mip levels:
            0 - 128x128
            1 - 64x64
            2 - 32x32
            3 - 16x16
            4 - 8x8
    */
    uint32_t nMipLevels = 5;

    GPUFormat cubemapFormat = GPUFormat::RGBA32_FLOAT;
    GPUFormat brdfFormat = GPUFormat::RG16_FLOAT;

    /* Generate irradiance map */
    TextureCreateInfo cubemapInfo = { };
    cubemapInfo.format = cubemapFormat;
    cubemapInfo.imageType = ETextureDimensions::TYPE_2D;
    cubemapInfo.extent.width = nIrradianceSize;
    cubemapInfo.extent.height = nIrradianceSize;
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

    cubemapInfo.extent.width = nPrefilterSize;
    cubemapInfo.extent.height = nPrefilterSize;
    cubemapInfo.nMipLevels = nMipLevels;

    this->m_prefilter = this->m_device->CreateTexture(cubemapInfo);

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


    /* Write descriptor sets */
    DescriptorImageInfo irradianceInfo = { };
    irradianceInfo.texture = this->m_skybox;
    irradianceInfo.imageView = this->m_skyboxView;
    irradianceInfo.sampler = this->m_sampler;

    DescriptorImageInfo prefilterInfo = { };
    prefilterInfo.texture = this->m_skybox;
    prefilterInfo.imageView = this->m_skyboxView;
    prefilterInfo.sampler = this->m_sampler;

    this->m_irradianceSet->WriteTexture(0, 0, irradianceInfo);
    this->m_prefilterSet->WriteTexture(0, 0, prefilterInfo);

    this->m_irradianceSet->UpdateWrites();
    this->m_prefilterSet->UpdateWrites();
    
    /* Create irradiance framebuffers */
    Vector<Ref<Framebuffer>> irradianceFBs(6);
    Vector<Ref<ImageView>> irradianceViews(6);
    for (uint32_t nFace = 0; nFace < 6; nFace++) {
        ImageViewCreateInfo viewInfo = { };
        viewInfo.image = this->m_irradiance;
        viewInfo.viewType = EImageViewType::TYPE_2D;
        viewInfo.format = cubemapFormat;
        viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
        viewInfo.subresourceRange.nBaseMipLevel = 0;
        viewInfo.subresourceRange.nLevelCount = 1;
        viewInfo.subresourceRange.nBaseArrayLayer = nFace;
        viewInfo.subresourceRange.nLayerCount = 1;
        
        irradianceViews[nFace] = this->m_device->CreateImageView(viewInfo);

        FramebufferCreateInfo fbInfo = { };
        fbInfo.renderPass = this->m_irradianceRP;
        fbInfo.attachments = Vector{ irradianceViews[nFace] };
        fbInfo.nLayers = 1;
        fbInfo.nWidth = nIrradianceSize;
        fbInfo.nHeight = nIrradianceSize;

        irradianceFBs[nFace] = this->m_device->CreateFramebuffer(fbInfo);
    }

    this->m_genViews.insert(this->m_genViews.end(), irradianceViews.begin(), irradianceViews.end());
    this->m_genFramebuffers.insert(this->m_genFramebuffers.end(), irradianceFBs.begin(), irradianceFBs.end());

    /* Render each irradiance map face */
    ClearValue clearValue{ { 0.f, 0.f, 0.f, 1.f } };

    DescriptorBufferInfo viewProjInfo = { };
    viewProjInfo.buffer = this->m_viewProjBuffer->GetBuffer();
    viewProjInfo.nOffset = 0;
    viewProjInfo.nRange = sizeof(ViewProjection);

    this->m_viewProjSet->WriteBuffer(0, 0, viewProjInfo);
    this->m_viewProjSet->UpdateWrites();

    for (uint32_t nFace = 0; nFace < 6; nFace++) {        
        RenderPassBeginInfo beginInfo = { };
        beginInfo.renderPass = this->m_irradianceRP;
        beginInfo.framebuffer = irradianceFBs[nFace];
        beginInfo.renderArea.extent = { nIrradianceSize, nIrradianceSize };
        beginInfo.clearValues = Vector{ clearValue };

        context->BeginRenderPass(beginInfo);
        
        Viewport viewport = { };
        viewport.width = static_cast<float>(nIrradianceSize);
        viewport.height = -static_cast<float>(nIrradianceSize);
        viewport.x = 0.f;
        viewport.y = static_cast<float>(nIrradianceSize);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        Rect2D scissor = { };
        scissor.extent = { nIrradianceSize, nIrradianceSize };
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

    /* Render each prefilter mip level */
    for (uint32_t nMip = 0; nMip < nMipLevels; nMip++) {
        uint32_t nMipSize = nPrefilterSize >> nMip; // 128, 64, 32, 16, 8
        float roughness = static_cast<float>(nMip) / static_cast<float>(nMipLevels - 1);


        /* Create framebuffers */
        Vector<Ref<Framebuffer>> framebuffers(6);
        Vector<Ref<ImageView>> prefilterViews(6);
        for (uint32_t nFace = 0; nFace < 6; nFace++) {
            ImageViewCreateInfo viewInfo = { };
            viewInfo.image = this->m_prefilter;
            viewInfo.viewType = EImageViewType::TYPE_2D;
            viewInfo.format = cubemapFormat;
            viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
            viewInfo.subresourceRange.nBaseArrayLayer = nFace;
            viewInfo.subresourceRange.nLayerCount = 1;
            viewInfo.subresourceRange.nBaseMipLevel = nMip;
            viewInfo.subresourceRange.nLevelCount = 1;

            prefilterViews[nFace] = this->m_device->CreateImageView(viewInfo);

            FramebufferCreateInfo fbInfo = { };
            fbInfo.renderPass = this->m_prefilterRP;
            fbInfo.nWidth = nMipSize;
            fbInfo.nHeight = nMipSize;
            fbInfo.attachments = Vector{ prefilterViews[nFace] };
            fbInfo.nLayers = 1;
            
            framebuffers[nFace] = this->m_device->CreateFramebuffer(fbInfo);
        }

        this->m_genViews.insert(this->m_genViews.end(), prefilterViews.begin(), prefilterViews.end());
        this->m_genFramebuffers.insert(this->m_genFramebuffers.end(), framebuffers.begin(), framebuffers.end());


        /* Render */
        for (uint32_t nFace = 0; nFace < 6; nFace++) {
            RenderPassBeginInfo beginInfo = { };
            beginInfo.renderPass = this->m_prefilterRP;
            beginInfo.framebuffer = framebuffers[nFace];
            beginInfo.renderArea.extent = { nMipSize, nMipSize };
            beginInfo.clearValues = Vector{ clearValue };
            
            context->BeginRenderPass(beginInfo);

            context->BindPipeline(this->m_prefilterPipeline);

            /* Copy view projection to uniform buffer */
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

    /* Generate BRDF LUT */
    TextureCreateInfo brdfInfo = { };
    brdfInfo.format = brdfFormat;
    brdfInfo.imageType = ETextureDimensions::TYPE_2D;
    brdfInfo.samples = ESampleCount::SAMPLE_1;
    brdfInfo.nMipLevels = 1;
    brdfInfo.nArrayLayers = 1;
    brdfInfo.extent = { nBRDFSize, nBRDFSize, 1 };
    brdfInfo.sharingMode = ESharingMode::EXCLUSIVE;
    brdfInfo.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
    brdfInfo.initialLayout = ETextureLayout::UNDEFINED;
    brdfInfo.tiling = ETextureTiling::OPTIMAL;
    
    this->m_brdf = this->m_device->CreateTexture(brdfInfo);

    context->ImageBarrier(this->m_brdf, EImageLayout::UNDEFINED, EImageLayout::COLOR_ATTACHMENT);

    /* Create a temporal image view for the framebuffer */
    ImageViewCreateInfo viewInfo = { };
    viewInfo.format = brdfFormat;
    viewInfo.image = this->m_brdf;
    viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    viewInfo.viewType = EImageViewType::TYPE_2D;
    viewInfo.subresourceRange.nBaseArrayLayer = 0;
    viewInfo.subresourceRange.nLayerCount = 1;
    viewInfo.subresourceRange.nBaseMipLevel = 0;
    viewInfo.subresourceRange.nLevelCount = 1;

    Ref<ImageView> brdfView = this->m_device->CreateImageView(viewInfo);
    this->m_genViews.push_back(brdfView);

    FramebufferCreateInfo brdfFBInfo = { };
    brdfFBInfo.nWidth = nBRDFSize;
    brdfFBInfo.nHeight = nBRDFSize;
    brdfFBInfo.renderPass = this->m_brdfRP;
    brdfFBInfo.nLayers = 1;
    brdfFBInfo.attachments = Vector{ brdfView };

    Ref<Framebuffer> brdfFB = this->m_device->CreateFramebuffer(brdfFBInfo);
    this->m_genFramebuffers.push_back(brdfFB);

    RenderPassBeginInfo beginInfo = { };
    beginInfo.framebuffer = brdfFB;
    beginInfo.renderPass = this->m_brdfRP;
    beginInfo.clearValues = Vector{ clearValue };
    beginInfo.renderArea.extent = { nBRDFSize, nBRDFSize };
    
    context->BeginRenderPass(beginInfo);

    Viewport viewport = { };
    viewport.width = static_cast<float>(nBRDFSize);
    viewport.height = static_cast<float>(nBRDFSize);
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    Rect2D scissor = { };
    scissor.extent = { nBRDFSize, nBRDFSize };
    scissor.offset = { 0, 0 };

    context->SetViewport(viewport);
    context->SetScissor(scissor);

    context->BindVertexBuffers(Vector{ this->m_sqVBO });
    context->BindIndexBuffer(this->m_sqIBO);
    context->BindPipeline(this->m_brdfPipeline);

    context->DrawIndexed(6);

    context->EndRenderPass();

    context->ImageBarrier(
        this->m_brdf,
        EImageLayout::COLOR_ATTACHMENT,
        EImageLayout::SHADER_READ_ONLY
    );

    /* Create image views */
    ImageViewCreateInfo cubemapViewInfo = { };
    cubemapViewInfo.format = cubemapFormat;
    cubemapViewInfo.image = this->m_irradiance;
    cubemapViewInfo.viewType = EImageViewType::TYPE_CUBE;
     
    cubemapViewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    cubemapViewInfo.subresourceRange.nBaseArrayLayer = 0;
    cubemapViewInfo.subresourceRange.nLayerCount = 6;
    cubemapViewInfo.subresourceRange.nBaseMipLevel = 0;
    cubemapViewInfo.subresourceRange.nLevelCount = 1;

    this->m_irradianceView = this->m_device->CreateImageView(cubemapViewInfo);

    cubemapViewInfo.image = this->m_prefilter;
    cubemapViewInfo.subresourceRange.nLevelCount = nMipLevels;

    this->m_prefilterView = this->m_device->CreateImageView(cubemapViewInfo);

    ImageViewCreateInfo brdfViewInfo = { };
    brdfViewInfo.format = brdfFormat;
    brdfViewInfo.image = this->m_brdf;
    brdfViewInfo.viewType = EImageViewType::TYPE_2D;

    brdfViewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    brdfViewInfo.subresourceRange.nBaseArrayLayer = 0;
    brdfViewInfo.subresourceRange.nLayerCount = 1;
    brdfViewInfo.subresourceRange.nBaseMipLevel = 0;
    brdfViewInfo.subresourceRange.nLevelCount = 1;

    this->m_brdfView = this->m_device->CreateImageView(brdfViewInfo);
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
    irradianceAttachment.format = GPUFormat::RGBA32_FLOAT;
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
    prefilterAttachment.format = GPUFormat::RGBA32_FLOAT;
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

    /* Prefilter render pass */
    AttachmentDescription brdfAttachment = { };
    brdfAttachment.format = GPUFormat::RG16_FLOAT;
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
    blendState.attachments = Vector{ colorBlend };
    
    DepthStencilState dsState = { };
    dsState.bDepthTestEnable = false;
    dsState.bDepthWriteEnable = false;
    dsState.bStencilTestEnable = false;

    /* Push constants (View + Projection) */
    PushConstantRange pushRange = { };
    pushRange.nSize = sizeof(glm::mat4) * 2;
    pushRange.nOffset = 0;
    pushRange.stage = EShaderStage::VERTEX;

    /* Irradiance pipeline */
    VertexInputBinding irradianceBindingDesc = { };
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

    PipelineLayoutCreateInfo irradianceLayoutInfo = { };
    irradianceLayoutInfo.setLayouts = { this->m_irradianceSetLayout };
    irradianceLayoutInfo.pushConstantRanges = { pushRange };

    this->m_irradianceLayout = this->m_device->CreatePipelineLayout(irradianceLayoutInfo);
    
    GraphicsPipelineCreateInfo irradiancePipelineInfo = { };
    irradiancePipelineInfo.colorBlendState = blendState;
    irradiancePipelineInfo.vertexBindings = { irradianceBindingDesc };
    irradiancePipelineInfo.vertexAttributes = { irradianceAttrib };
    irradiancePipelineInfo.nSubpass = 0;
    irradiancePipelineInfo.colorFormats = { GPUFormat::RGBA32_FLOAT };
    irradiancePipelineInfo.rasterizationState = iblRaster;
    irradiancePipelineInfo.multisampleState = iblMS;
    irradiancePipelineInfo.shaders = { irradianceVS, irradiancePS };
    irradiancePipelineInfo.pipelineLayout = this->m_irradianceLayout;
    irradiancePipelineInfo.topology = EPrimitiveTopology::TRIANGLE_LIST;
    irradiancePipelineInfo.depthStencilState = dsState;
    irradiancePipelineInfo.renderPass = this->m_irradianceRP;
    
    this->m_irradiancePipeline = this->m_device->CreateGraphicsPipeline(irradiancePipelineInfo);

    /* Prefilter pipeline */
    VertexInputBinding prefilterBindingDesc = { };
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
    prefilterPipelineInfo.colorFormats = { GPUFormat::RGBA32_FLOAT };
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
    brdfPipelineInfo.colorFormats = { GPUFormat::RG16_FLOAT };
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

    /* Create view projection buffer */
    uint32_t nViewProjectionSize = sizeof(ViewProjection);

    RingBufferCreateInfo viewProjInfo = { };
    viewProjInfo.nAlignment = nViewProjectionSize;
    viewProjInfo.nFramesInFlight = 1;
    viewProjInfo.nBufferSize = nViewProjectionSize * 36; // One per cube face (6 for irradiance + 30 for prefilter)
    viewProjInfo.usage = EBufferUsage::UNIFORM_BUFFER;

    this->m_viewProjBuffer = this->m_device->CreateRingBuffer(viewProjInfo);
}