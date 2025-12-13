#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "Renderer/Renderer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Input.h"

/* Forward declarations */
class SceneManager;

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

    SceneManager* m_sceneMgr;

    Renderer* m_renderer;
};