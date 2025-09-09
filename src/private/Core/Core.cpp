#include "Core/Core.h"

Core* Core::m_instance;

/* Core constructor */
Core::Core() {
    this->m_pWindow = nullptr;
    this->m_renderer = nullptr;
}

/* Core init method */
void Core::Init() {
    /* Initialize GLFW */
    if (!glfwInit()) {
        spdlog::error("Error initializing GLFW");
        return;
    }

    /* 
        GLFW Window hints:
            We don't want OpenGL. WE WANT VULKAN!!!!
            We don't want to resize our window (for the moment)
    */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->m_pWindow = glfwCreateWindow(800, 600, "Aetherion Engine", nullptr, nullptr); // GLFW Window creation

    /* Assert window is not null */
    if (this->m_pWindow == nullptr) {
        spdlog::error("Window not initialized");
        throw std::runtime_error("Window not initialized");
        return;
    }

    /* 
        Create a new Vulkan renderer instance.
           
        TODO: Change between graphics APIs.
            Note: When implemented many APIs and not only Vulkan.
    */
    this->m_renderer = new VulkanRenderer();

    /* Set our renderer window and init */
    this->m_renderer->SetWindow(this->m_pWindow);
    this->m_renderer->Init();
}

/* Our core update method */
void Core::Update() {
    /* While window should not close */
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        glfwPollEvents(); // Poll GLFW events
        this->m_renderer->Update(); // Update our renderer
    }
}

Core *Core::GetInstance() {
    if (Core::m_instance == nullptr)
        Core::m_instance = new Core();
    return Core::m_instance;
}
