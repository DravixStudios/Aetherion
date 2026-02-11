#include "Core/Renderer/Rendering/DeferredRenderer.h"

/**
* Deferred renderer initialization
*/
void
DeferredRenderer::Init(Ref<Device> device, Ref<Swapchain> swapchain, uint32_t nFramesInFlight) {
    this->m_device = device;
    this->m_nFramesInFlight = nFramesInFlight;
    
    Extent2D ext = swapchain->GetExtent();
    if (ext.width == 0 || ext.height == 0) {
        Logger::Warn("DeferredRenderer::Init: Swapchain extent is 0x0, forcing 1x1");
        ext.width = 1;
        ext.height = 1;
    }

    Logger::Info("DeferredRenderer::Init: Initializing with dimensions {}x{}", ext.width, ext.height);

    /* Render graph initialization */
    this->m_graph.Setup(device, nFramesInFlight);

    this->CreateScreenquadBuffer();
    this->LoadSkybox();
    this->m_iblGen.Init(device, this->m_skybox, this->m_skyboxView, this->m_cubeSampler, this->m_sqVBO, this->m_sqIBO);

    /* Initialize passes */
    this->m_cullingPass.Init(device, this->m_nFramesInFlight);

    this->m_gbuffPass.Init(device);
    this->m_gbuffPass.Resize(ext.width, ext.height);

    this->m_lightingPass.Init(device, this->m_nFramesInFlight);
    this->m_lightingPass.SetDimensions(ext.width, ext.height);
    this->m_lightingPass.SetGBufferDescriptorSet(this->m_gbuffPass.GetReadDescriptorSet());

    this->m_skyboxPass.Init(device);
    this->m_skyboxPass.SetDimensions(ext.width, ext.height);

    this->m_tonemapPass.Init(device, swapchain, this->m_nFramesInFlight);
    this->m_tonemapPass.SetDimensions(ext.width, ext.height);
    this->m_tonemapPass.SetScreenQuad(this->m_sqVBO, this->m_sqIBO, 6);

    /* Create bindless resources */
    this->CreateBindlessResources();
    this->CreateSceneDescriptors();
    this->CreateSkyboxDescriptors();
    this->UpdateSkyboxDescriptor();

    /* MegaBuffer and MeshUploader initialization */
    this->m_megaBuffer.Init(device, 1024 * 1024, 4 * 1024 * 1024);
    this->m_meshUploader.Init(device, &this->m_megaBuffer, this->m_bindlessSet, this->m_defaultSampler);

}

/**
* Resizes the passes and the g-buffer manager
* 
* @param nWidth Width
* @param nHeight Height
*/
void
DeferredRenderer::Resize(uint32_t nWidth, uint32_t nHeight) {
    if (nWidth == 0 || nHeight == 0) {
        return;
    }

    this->m_gbuffPass.Resize(nWidth, nHeight);
    this->m_lightingPass.SetDimensions(nWidth, nHeight);
    this->m_skyboxPass.SetDimensions(nWidth, nHeight);
    this->m_tonemapPass.SetDimensions(nWidth, nHeight);

    this->m_lightingPass.SetGBufferDescriptorSet(this->m_gbuffPass.GetReadDescriptorSet());
    this->UpdateSkyboxDescriptor();
}

/**
* Executes the full deferred rendering pipeline
* 
* @param context Graphics context
* @param swapchain Swapchain
* @param drawData Collected scene data for drawing
* @param nImgIdx Current image index for frames-in-flight
*/
void
DeferredRenderer::Render(
	Ref<GraphicsContext> context,
	Ref<Swapchain> swapchain,
	const CollectedDrawData& drawData,
	uint32_t nImgIdx
) {
    this->m_graph.Reset(nImgIdx);

    if (!this->m_bIBLGenerated) {
        this->m_iblGen.Generate(context);
        this->m_bIBLGenerated = true;
    }

    TextureHandle backBuffer = this->m_graph.ImportBackbuffer(
        swapchain->GetImage(nImgIdx),
        swapchain->GetImageView(nImgIdx)
    );

    this->UploadSceneData(drawData, nImgIdx);
   

    this->m_cullingPass.SetViewProj(drawData.viewProj);

    this->UpdateSceneDescriptors(nImgIdx);
    this->m_gbuffPass.SetSceneData(
        this->m_sceneSets[nImgIdx],
        this->m_sceneSetLayout,
        this->m_bindlessSet,
        this->m_bindlessLayout,
        this->m_megaBuffer.GetVertexBuffer(),
        this->m_megaBuffer.GetIndexBuffer(),
        0,
        this->m_cullingPass.GetCountBuffer(),
        this->m_cullingPass.GetIndirectBuffer()->GetBuffer()
    );

    /* Import G-Buffer resources */
    this->m_gbuffPass.ImportResources(this->m_graph);

    this->m_graph.AddNode("Culling",
        [&](RenderGraphBuilder& builder) { this->m_cullingPass.SetupNode(builder); },
        [&](Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
            this->m_cullingPass.Execute(context, graphCtx, nImgIdx);
        }
    );

    /* 1. G-Buffer pass (Indirect) */
    this->m_graph.AddNode("GBuffer",
        [&](RenderGraphBuilder& builder) { this->m_gbuffPass.SetupNode(builder); },
        [&](Ref<GraphicsContext> context, RenderGraphContext& graphCtx) { 
            this->m_gbuffPass.Execute(context, graphCtx);
        }
    );

    /* 2. Lighting pass (HDR) */
    this->m_lightingPass.SetInput(this->m_gbuffPass.GetOutput());
    this->m_lightingPass.SetCameraPosition(drawData.cameraPosition);
    
    // Set light data
    this->m_lightingPass.SetLightData(
        this->m_iblGen.GetIrradianceView(),
        this->m_iblGen.GetPrefilterView(),
        this->m_iblGen.GetBRDFView(),
        this->m_sqVBO,
        this->m_sqIBO,
        this->m_cubeSampler,
        this->m_defaultSampler,
        6
    );

    this->m_lightingPass.SetGBufferDescriptorSet(this->m_gbuffPass.GetReadDescriptorSet());

    this->m_graph.AddNode("Lighting",
        [&](RenderGraphBuilder& builder) { this->m_lightingPass.SetupNode(builder); },
        [&](Ref<GraphicsContext> context, RenderGraphContext& graphCtx) { 
            this->m_lightingPass.Execute(context, graphCtx); 
        }
    );

    /* 3. Skybox pass */
    if (this->m_lightingPass.GetOutput().hdrOutput.IsValid()) {
         this->m_skyboxPass.SetInput({ this->m_gbuffPass.GetOutput().depth, this->m_lightingPass.GetOutput().hdrOutput});

        this->m_skyboxPass.SetSkyboxData(
            this->m_skyboxSet,
            this->m_skyboxSetLayout,
            this->m_sqVBO,
            this->m_sqIBO,
            6
        );

        this->m_skyboxPass.UpdateCamera(drawData.view, drawData.proj, drawData.cameraPosition);

        this->m_graph.AddNode("Skybox",
            [&](RenderGraphBuilder& builder) { this->m_skyboxPass.SetupNode(builder); },
            [&](Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
                this->m_skyboxPass.Execute(context, graphCtx);
            }
        );
    }

    /* 4. Tonemap pass (Final LDR presentation to backbuffer) */
    this->m_tonemapPass.SetInput(this->m_lightingPass.GetOutput().hdrOutput);
    this->m_tonemapPass.SetOutput(backBuffer);
    
    this->m_graph.AddNode("Tonemap",
        [&](RenderGraphBuilder& builder) { this->m_tonemapPass.SetupNode(builder); },
        [&](Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
            this->m_tonemapPass.Execute(context, graphCtx, nImgIdx);
        }
    );

    this->m_graph.Compile();
    this->m_graph.Execute(context);
}

/**
* Uploads scene transformation and instance data to GPU buffers
* 
* @param data Draw data
* @param nFrameIdx Current frame index
*/
void 
DeferredRenderer::UploadSceneData(const CollectedDrawData& data, uint32_t nFrameIdx) {
    if (data.batches.empty()) return;

    /* Reset ring buffers */
    this->m_cullingPass.GetInstanceBuffer()->Reset(nFrameIdx);
    this->m_cullingPass.GetBatchBuffer()->Reset(nFrameIdx);
    this->m_cullingPass.GetWVPBuffer()->Reset(nFrameIdx);

    /* Copy data to ring buffers */
    uint32_t nOffset = 0;

    /* Object instance data */
    void* ptr = this->m_cullingPass.GetInstanceBuffer()->Allocate(
        static_cast<uint32_t>(data.instances.size() * sizeof(ObjectInstanceData)),
        nOffset
    );

    memcpy(ptr, data.instances.data(), data.instances.size() * sizeof(ObjectInstanceData));

    /* Draw batches */
    ptr = this->m_cullingPass.GetBatchBuffer()->Allocate(
        static_cast<uint32_t>(data.batches.size() * sizeof(DrawBatch)),
        nOffset
    );

    memcpy(ptr, data.batches.data(), data.batches.size() * sizeof(DrawBatch));

    /* WVP */
    ptr = this->m_cullingPass.GetWVPBuffer()->Allocate(
        static_cast<uint32_t>(data.wvps.size() * sizeof(WVP)),
        nOffset
    );

    memcpy(ptr, data.wvps.data(), data.wvps.size() * sizeof(WVP));

    this->m_cullingPass.SetTotalBatches(data.nTotalBatches);
}

/**
* Creates descriptor set layout and pool for skybox rendering
*/
void
DeferredRenderer::CreateSkyboxDescriptors() {
    Vector<DescriptorSetLayoutBinding> bindings = {
        { 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT },
        { 1, EDescriptorType::COMBINED_IMAGE_SAMPLER, 1, EShaderStage::FRAGMENT }
    };

    DescriptorSetLayoutCreateInfo layoutInfo = { };
    layoutInfo.bindings = bindings;
    this->m_skyboxSetLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

    DescriptorPoolCreateInfo poolInfo = { };
    poolInfo.nMaxSets = 1;
    poolInfo.poolSizes = {
        { EDescriptorType::COMBINED_IMAGE_SAMPLER, 2 }
    };
    this->m_skyboxPool = this->m_device->CreateDescriptorPool(poolInfo);

    this->m_skyboxSet = this->m_device->CreateDescriptorSet(this->m_skyboxPool, this->m_skyboxSetLayout);
}

/**
* Updates skybox descriptor set with textures
*/
void
DeferredRenderer::UpdateSkyboxDescriptor() {
    /* Skybox texture */
    DescriptorImageInfo skyboxInfo = { };
    skyboxInfo.imageView = this->m_skyboxView;
    skyboxInfo.sampler = this->m_cubeSampler;
    skyboxInfo.texture = this->m_skybox;

    /* Depth texture (from GBuffer) */
    DescriptorImageInfo depthInfo = { };
    depthInfo.imageView = this->m_gbuffPass.GetDepthView();
    depthInfo.sampler = this->m_defaultSampler;
    depthInfo.texture = this->m_gbuffPass.GetDepthView()->GetImage();

    this->m_skyboxSet->WriteTexture(0, 0, skyboxInfo);
    this->m_skyboxSet->WriteTexture(1, 0, depthInfo);
    this->m_skyboxSet->UpdateWrites();
}

/**
* Initializes bindless resource infrastructure (pool, layout, set, sampler)
*/
void
DeferredRenderer::CreateBindlessResources() {
    /* Descriptor pool for bindless textures */
    DescriptorPoolCreateInfo poolInfo = { };
    poolInfo.nMaxSets = 1;
    poolInfo.poolSizes = {
        { EDescriptorType::COMBINED_IMAGE_SAMPLER, 4096 }
    };
    poolInfo.bUpdateAfterBind = true;
    this->m_bindlessPool = this->m_device->CreateDescriptorPool(poolInfo);

    /* Bindless descriptor set layout */
    DescriptorSetLayoutCreateInfo layoutInfo = { };
    layoutInfo.bindings = {
        { 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 4096, EShaderStage::FRAGMENT, true }
    };
    this->m_bindlessLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

    /* Bindless descriptor set */
    this->m_bindlessSet = this->m_device->CreateDescriptorSet(this->m_bindlessPool, this->m_bindlessLayout);

    /* Default sampler */
    SamplerCreateInfo samplerInfo = { };
    samplerInfo.magFilter = EFilter::LINEAR;
    samplerInfo.minFilter = EFilter::LINEAR;
    samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = EAddressMode::REPEAT;
    samplerInfo.addressModeV = EAddressMode::REPEAT;
    samplerInfo.addressModeW = EAddressMode::REPEAT;
    samplerInfo.bAnisotropyEnable = true;
    samplerInfo.maxAnisotropy = 16.f;
    this->m_defaultSampler = this->m_device->CreateSampler(samplerInfo);
}

/**
* Uploads a mesh to the global mega-buffer
* 
* @param meshData Mesh source data
*/
void
DeferredRenderer::UploadMesh(const MeshData& meshData) {
    if (!meshData.bLoaded) return;

    const String& name = meshData.name;
    if (this->m_uploadedMeshes.count(name) > 0) return;

    UploadedMesh uploaded = this->m_meshUploader.Upload(meshData);
    this->m_uploadedMeshes[name] = uploaded;
}

/**
* Initializes scene-wide descriptor sets and layouts (transformations, instances)
*/
void
DeferredRenderer::CreateSceneDescriptors() {
    /*
        Scene Descriptor layout (Set 0)
        Binding 0: Instance Data (Storage Buffer)
        Binding 4: WVP Data (Storage buffer)
    */
    Vector<DescriptorSetLayoutBinding> bindings = {
        { 0, EDescriptorType::STORAGE_BUFFER, 1, EShaderStage::VERTEX | EShaderStage::FRAGMENT, false },
        { 4, EDescriptorType::STORAGE_BUFFER, 1, EShaderStage::VERTEX, false }
    };

    /* Create descriptor set layout */
    DescriptorSetLayoutCreateInfo layoutInfo = { };
    layoutInfo.bindings = bindings;
    
    this->m_sceneSetLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

    /* Create Descriptor pool */
    DescriptorPoolCreateInfo poolInfo = { };
    poolInfo.nMaxSets = this->m_nFramesInFlight;
    poolInfo.poolSizes = {
        { EDescriptorType::STORAGE_BUFFER, 2 * this->m_nFramesInFlight }
    };

    this->m_scenePool = this->m_device->CreateDescriptorPool(poolInfo);

    /* Allocate sets */
    this->m_sceneSets.resize(this->m_nFramesInFlight);
    for (uint32_t i = 0; i < this->m_nFramesInFlight; i++) {
        this->m_sceneSets[i] = this->m_device->CreateDescriptorSet(this->m_scenePool, this->m_sceneSetLayout);
    }
}

/**
* Updates scene descriptor sets for a specific frame index
* 
* @param nImgIdx Frame/Image index
*/
void
DeferredRenderer::UpdateSceneDescriptors(uint32_t nImgIdx) {
    Ref<DescriptorSet> currentSceneSet = this->m_sceneSets[nImgIdx];

    /* Binding 0: Instance data (from CullingPass) */
    DescriptorBufferInfo instanceInfo = { };
    instanceInfo.buffer = this->m_cullingPass.GetInstanceBuffer()->GetBuffer();
    instanceInfo.nOffset = 0;
    instanceInfo.nRange = 0; // WHOLE SIZE

    /* Binding 4: WVP Data (from CullingPass) */
    DescriptorBufferInfo wvpInfo = { };
    wvpInfo.buffer = this->m_cullingPass.GetWVPBuffer()->GetBuffer();
    wvpInfo.nOffset = 0;
    wvpInfo.nRange = 0;
    
    currentSceneSet->WriteBuffer(0, 0, instanceInfo);
    currentSceneSet->WriteBuffer(4, 0, wvpInfo);
    currentSceneSet->UpdateWrites();
}

/**
* Loads the skybox
*/
void
DeferredRenderer::LoadSkybox() {
    void* pData = nullptr;
    uint32_t nSize = 0;
    uint32_t nFaceSize = 0;
    bool bSkyboxLoaded = LoadCubemap("ferndale_studio_04_4k.exr", &pData, nSize, nFaceSize);
    if (!bSkyboxLoaded) {
        Logger::Error("DeferredRenderer::Init: Failed loading skybox");
        throw std::runtime_error("DeferredRenderer::Init Failed loading skybox");
    }

    /* Create staging buffer */
    BufferCreateInfo stagingInfo = { };
    stagingInfo.nSize = nSize;
    stagingInfo.pcData = pData;
    stagingInfo.sharingMode = ESharingMode::EXCLUSIVE;
    stagingInfo.type = EBufferType::STAGING_BUFFER;
    stagingInfo.usage = EBufferUsage::TRANSFER_SRC;
    
    Ref<GPUBuffer> stagingBuff = this->m_device->CreateBuffer(stagingInfo);

    /* Create texture */
    TextureCreateInfo textureInfo = { };
    textureInfo.buffer = stagingBuff;
    textureInfo.extent = { nFaceSize, nFaceSize, 1 };
    textureInfo.format = GPUFormat::RGBA32_FLOAT;
    textureInfo.nMipLevels = 1;
    textureInfo.nArrayLayers = 6;
    textureInfo.samples = ESampleCount::SAMPLE_1;
    textureInfo.usage = ETextureUsage::TRANSFER_DST | ETextureUsage::SAMPLED;
    textureInfo.sharingMode = ESharingMode::EXCLUSIVE;
    textureInfo.tiling = ETextureTiling::OPTIMAL;
    textureInfo.imageType = ETextureDimensions::TYPE_2D;
    textureInfo.initialLayout = ETextureLayout::UNDEFINED;
    textureInfo.flags = ETextureFlags::CUBE_COMPATIBLE;

    this->m_skybox = this->m_device->CreateTexture(textureInfo);

    /* Create skybox image view */
    ImageViewCreateInfo viewInfo = { };
    viewInfo.viewType = EImageViewType::TYPE_CUBE;
    viewInfo.image = this->m_skybox;
    viewInfo.format = GPUFormat::RGBA32_FLOAT;
    viewInfo.subresourceRange.aspectMask = EImageAspect::COLOR;
    viewInfo.subresourceRange.nBaseArrayLayer = 0;
    viewInfo.subresourceRange.nLayerCount = 6;
    viewInfo.subresourceRange.nBaseMipLevel = 0;
    viewInfo.subresourceRange.nLevelCount = 1;

    this->m_skyboxView = this->m_device->CreateImageView(viewInfo);

    /* Create cube sampler */
    SamplerCreateInfo samplerInfo = { };
    samplerInfo.addressModeU = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.addressModeV = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.addressModeW = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.minFilter = EFilter::LINEAR;
    samplerInfo.magFilter = EFilter::LINEAR;
    samplerInfo.bAnisotropyEnable = false;
    samplerInfo.maxAnisotropy = 1.f;
    samplerInfo.bCompareEnable = false;
    samplerInfo.borderColor = EBorderColor::FLOAT_OPAQUE_WHITE;
    samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 1.f;

    this->m_cubeSampler = this->m_device->CreateSampler(samplerInfo);
}

/**
* Initializes the vertex and index buffers for a screen-aligned quad
*/
void
DeferredRenderer::CreateScreenquadBuffer() {
    Vector<ScreenQuadVertex> sqVertices = {
        { { -1.f, 1.f, 0.f }, { 0.f, 0.f } },
        { { 1.f, 1.f, 0.f }, { 1.f, 0.f } },
        { { -1.f, -1.f, 0.f }, { 0.f, 1.f } },
        { { 1.f, -1.f, 0.f }, { 1.f, 1.f } }
    };

    Vector<uint16_t> sqIndices = {
        0, 1, 2,
        2, 1, 3
    };

    BufferCreateInfo vboInfo = { };
    vboInfo.nSize = sqVertices.size() * sizeof(ScreenQuadVertex);
    vboInfo.pcData = sqVertices.data();
    vboInfo.sharingMode = ESharingMode::EXCLUSIVE;
    vboInfo.type = EBufferType::VERTEX_BUFFER;
    vboInfo.usage = EBufferUsage::VERTEX_BUFFER;
    
    this->m_sqVBO = this->m_device->CreateBuffer(vboInfo);

    BufferCreateInfo iboInfo = { };
    iboInfo.nSize = sqIndices.size() * sizeof(uint16_t);
    iboInfo.pcData = sqIndices.data();
    iboInfo.sharingMode = ESharingMode::EXCLUSIVE;
    iboInfo.type = EBufferType::INDEX_BUFFER;
    iboInfo.usage = EBufferUsage::INDEX_BUFFER;

    this->m_sqIBO = this->m_device->CreateBuffer(iboInfo);
}