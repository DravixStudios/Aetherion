#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Renderer {
protected:
    GLFWwindow* m_pWindow;
public:
    Renderer();
    virtual void Init();
    virtual void Update();
    virtual void SetWindow(GLFWwindow* pWindow);
};
