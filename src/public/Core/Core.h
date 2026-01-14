#pragma once
#include <iostream>
#include <spdlog/spdlog.h>
#include "Core/Input.h"
#include "Core/Time.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Renderer/Renderer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"

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

    Renderer* GetRenderer() const { return this->m_renderer; }

    static Core* GetInstance();
private:
    ERenderBackend m_renderBackend;

    GLFWwindow* m_pWindow;
    static Core* m_instance;

    Input* m_input;
    Time* m_time;

    SceneManager* m_sceneMgr;
    ResourceManager* m_resMgr;

    Renderer* m_renderer;
};