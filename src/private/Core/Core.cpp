#include "Core/Core.h"

Core* Core::m_instance;

Core::Core() {
    this->m_pWindow = nullptr;
}

void Core::Init() {
    if (!glfwInit()) {
        spdlog::error("Error initializing GLFW");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->m_pWindow = glfwCreateWindow(800, 600, "Aetherion Engine", nullptr, nullptr);

    if (this->m_pWindow == nullptr) {
        spdlog::error("Window not initialized");
        throw std::runtime_error("Window not initialized");
        return;
    }
}

void Core::Update() {
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        glfwPollEvents();
    }
}

Core *Core::GetInstance() {
    if (Core::m_instance == nullptr)
        Core::m_instance = new Core();
    return Core::m_instance;
}
