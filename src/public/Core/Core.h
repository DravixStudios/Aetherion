#pragma once
#include <iostream>
#include <spdlog/spdlog.h>
#include "Core/Input.h"
#include "Core/Time.h"

#include "Core/Containers.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/Renderer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"

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
};