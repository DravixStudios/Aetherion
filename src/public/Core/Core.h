#pragma once
#include <iostream>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

class Core {
public:
    Core();

    void Init();
    void Update();
    static Core* GetInstance();
private:
    GLFWwindow* m_pWindow;
    static Core* m_instance;
};