#include "Core/Core.h"
#include "Core/Scene/SceneManager.h"
#include "Core/Renderer/ResourceManager.h"

Core* Core::m_instance;

/* Core constructor */
Core::Core() {
    this->m_pWindow = nullptr;
    this->m_renderer = nullptr;
    this->m_sceneMgr = nullptr;
    this->m_input = Input::GetInstance();
    this->m_time = Time::GetInstance();
    this->m_resMgr = ResourceManager::GetInstance();
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

    /* TODO: Make window resizable */
    this->m_pWindow = glfwCreateWindow(1280, 720, "Aetherion Engine", nullptr, nullptr); // GLFW Window creation

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
    
    this->m_input->SetWindow(this->m_pWindow);
    glfwSetKeyCallback(this->m_pWindow, Input::KeyCallback);
    glfwSetMouseButtonCallback(this->m_pWindow, Input::MouseButtonCallback);

    this->m_resMgr->SetRenderer(this->m_renderer);
    this->m_sceneMgr = SceneManager::GetInstance();
    this->m_sceneMgr->Start();
}

/* Our core update method */
void Core::Update() {
    /* While window should not close */
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        this->m_time->PreUpdate();
        glfwPollEvents(); // Poll GLFW events
        this->m_sceneMgr->Update();
        this->m_renderer->Update(); // Update our renderer
        this->m_input->Close();
        this->m_time->PostUpdate();
    }
}

/* Get our renderer instance */
Renderer* Core::GetRenderer() {
    if (this->m_renderer == nullptr) {
        spdlog::error("Core::GetRenderer: Core::m_renderer class member not defined");
        throw std::runtime_error("Core::GetRenderer: Core::m_renderer class member not defined");
        return nullptr;
    }

    return this->m_renderer;
}

Core* Core::GetInstance() {
    if (Core::m_instance == nullptr)
        Core::m_instance = new Core();
    return Core::m_instance;
}
