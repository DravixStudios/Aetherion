#pragma once
#include <iostream>
#include <spdlog/spdlog.h>
#include "Core/Input.h"
#include "Core/Time.h"

#include "Core/Containers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Core/Renderer/Renderer.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/CommandPool.h"
#include "Core/Renderer/Swapchain.h"
#include "Core/Renderer/Rendering/DeferredRenderer.h"

#include "Core/Renderer/SceneCollector.h"

#include "Core/Renderer/Vulkan/VulkanRenderer.h"

#include "Core/Renderer/Semaphore.h"
#include "Core/Renderer/Fence.h"

/* TODO: Custom width */
#define WIDTH 1600
#define HEIGHT 900

/* Forward declarations */
class SceneManager;
class ResourceManager;

enum class ERenderBackend {
    VULKAN,

    /* TODO */
    D3D12,
    D2D11,
    OPENGL,
    METAL
};

class Core {
public:
    Core();

    void Init();
    void Update();

    Ref<Renderer> GetRenderer() const { return this->m_renderer; }

    static Core* GetInstance();
private:
    ERenderBackend m_renderBackend;

    GLFWwindow* m_pWindow;
    static Core* m_instance;

    Input* m_input;
    Time* m_time;

    SceneManager* m_sceneMgr;
    ResourceManager* m_resMgr;

    Ref<Renderer> m_renderer;

    Ref<Device> m_device;
    Ref<CommandPool> m_pool;
    Vector<Ref<GraphicsContext>> m_contexts;

    Ref<Swapchain> m_swapchain;

    ESampleCount m_sampleCount;
    uint32_t m_nImageCount;

    DeferredRenderer m_deferredRenderer;

    Vector<Ref<Semaphore>> m_imageAvailableSemaphores;
    Vector<Ref<Semaphore>> m_renderFinishedSemaphores;
    Vector<Ref<Fence>> m_inFlightFences;

    SceneCollector m_sceneCollector;

    uint32_t m_nImageIndex = 0;

    void CreateSwapchain();
    void CreateSyncObjects();
};