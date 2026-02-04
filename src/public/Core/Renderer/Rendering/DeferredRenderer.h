#pragma once
#include "Core/Renderer/Device.h"
#include "Core/Renderer/Swapchain.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Rendering/RenderGraph.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferManager.h"
#include "Core/Renderer/Rendering/Passes/GBufferPass.h"
#include "Core/Renderer/Rendering/Passes/LightingPass.h"

class DeferredRenderer {
public:
    void Init(Ref<Device> device, Ref<Swapchain> swapchain);
    void Resize(uint32_t nWidth, uint32_t nHeight);

    void Render(Ref<GraphicsContext> context, Ref<Swapchain> swapchain, uint32_t nImgIdx);

    GBufferPass& GetGBufferPass() { return this->m_gbuffPass; }
    LightingPass& GetLightingPass() { return this->m_lightingPass; }
private:
    Ref<Device> m_device;
    RenderGraph m_graph;
    
    GBufferManager m_gbuffer;
    GBufferPass m_gbuffPass;

    LightingPass m_lightingPass;
};