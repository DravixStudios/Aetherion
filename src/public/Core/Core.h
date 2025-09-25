#pragma once
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include "Renderer/Renderer.h"
#include "Renderer/Vulkan/VulkanRenderer.h"

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

    Renderer* m_renderer;
};