#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "Renderer/Renderer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Input.h"
#include "Core/Time.h"

/* Forward declarations */
class SceneManager;
class ResourceManager;

class Core {
public:
    Core();

    void Init();
    void Update();

    Renderer* GetRenderer();

    static Core* GetInstance();
private:
    GLFWwindow* m_pWindow;
    static Core* m_instance;

    Input* m_input;
    Time* m_time;

    SceneManager* m_sceneMgr;
    ResourceManager* m_resMgr;

    Renderer* m_renderer;
};