#pragma once
#include "Core/Renderer/Device.h"
#include "Core/Renderer/Swapchain.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Rendering/RenderGraph.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferManager.h"
#include "Core/Renderer/Rendering/Passes/CullingPass.h"
#include "Core/Renderer/Rendering/Passes/GBufferPass.h"
#include "Core/Renderer/Rendering/Passes/ShadowPass.h"
#include "Core/Renderer/Rendering/Passes/LightingPass.h"
#include "Core/Renderer/Rendering/Passes/SkyboxPass.h"
#include "Core/Renderer/Rendering/Passes/TonemapPass.h"
#include "Core/Renderer/Rendering/Passes/ImGuiPass.h"
#include "Core/Renderer/Rendering/Passes/BentNormalPass.h"
#include "Core/Renderer/Rendering/SunExtraction.h"

#include "Core/Renderer/Rendering/IBL/IBLGenerator.h"
#include "Core/Renderer/CubemapUtils.h"

#include "Core/Renderer/MegaBuffer.h"
#include "Core/Renderer/SceneCollector.h"
#include "Core/Renderer/MeshUploader.h"

#include "Core/Renderer/GPUBuffer.h"

#include <glm/glm.hpp>

class DeferredRenderer {
public:
    void Init(Ref<Device> device, Ref<Swapchain> swapchain, uint32_t nFramesInFlight, GLFWwindow* pWindow);
    void Resize(uint32_t nWidth, uint32_t nHeight);
    void Invalidate();

    void Render(
        Ref<GraphicsContext> context,
        Ref<Swapchain> swapchain,
        const CollectedDrawData& drawData,
        uint32_t nImgIdx
    );

    CullingPass& GetCullingPass() { return this->m_cullingPass; }
    GBufferPass& GetGBufferPass() { return this->m_gbuffPass; }
    LightingPass& GetLightingPass() { return this->m_lightingPass; }
    SkyboxPass& GetSkyboxPass() { return this->m_skyboxPass; }

    const std::map<String, UploadedMesh>& GetUploadedMeshes() const { return this->m_uploadedMeshes; }

    void UploadMesh(const MeshData& meshData);
    MegaBuffer& GetMegaBuffer() { return this->m_megaBuffer; }

private:
    Ref<Device> m_device;
    RenderGraph m_graph;

    CullingPass m_cullingPass;
    
    GBufferPass m_gbuffPass;
    ShadowPass m_shadowPass;
    LightingPass m_lightingPass;
    SkyboxPass m_skyboxPass;
    TonemapPass m_tonemapPass;
    ImGuiPass m_imguiPass;
    BentNormalPass m_bentNormalPass;

    IBLGenerator m_iblGen;
    SunExtraction m_sunExtraction;

    Ref<GPUBuffer> m_indirectBuffer;
    Ref<GPUBuffer> m_countBuffer;

    uint32_t m_nFramesInFlight = 0;

    Ref<DescriptorPool> m_bindlessPool;
    Ref<DescriptorSetLayout> m_bindlessLayout;
    Ref<DescriptorSet> m_bindlessSet;
    Ref<Sampler> m_defaultSampler;

    MegaBuffer m_megaBuffer;
    MeshUploader m_meshUploader;
    std::map<String, UploadedMesh> m_uploadedMeshes;

    Ref<DescriptorPool> m_scenePool;
    Ref<DescriptorSetLayout> m_sceneSetLayout;
    Vector<Ref<DescriptorSet>> m_sceneSets;

    Ref<GPUTexture> m_skybox;
    Ref<ImageView> m_skyboxView;
    Ref<Sampler> m_cubeSampler;

    Ref<GPUBuffer> m_sqVBO;
    Ref<GPUBuffer> m_sqIBO;
    
    Ref<DescriptorSetLayout> m_skyboxSetLayout;
    Ref<DescriptorPool> m_skyboxPool;
    Ref<DescriptorSet> m_skyboxSet;

    GLFWwindow* m_pWindow = nullptr;

    bool m_bIBLGenerated = false;

    void UploadSceneData(const CollectedDrawData& data, uint32_t nFrameIdx);
    void CreateBindlessResources();
    void CreateSceneDescriptors();
    
    void CreateSkyboxDescriptors();
    void UpdateSkyboxDescriptor();

    void UpdateSceneDescriptors(uint32_t nImgIdx);
    void LoadSkybox();
    void CreateScreenquadBuffer();
};