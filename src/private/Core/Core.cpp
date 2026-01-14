#include "Core/Core.h"
#include "Core/Scene/SceneManager.h"
#include "Core/Renderer/ResourceManager.h"

Core* Core::m_instance;

/* Core constructor */
Core::Core()
    : m_renderBackend(ERenderBackend::VULKAN), 
    m_resMgr(ResourceManager::GetInstance()), m_input(Input::GetInstance()) {}

/* Core init method */
void 
Core::Init() {
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
    this->m_pWindow = glfwCreateWindow(1600, 900, "Aetherion Engine", nullptr, nullptr); // GLFW Window creation

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

    switch (this->m_renderBackend) {
        case ERenderBackend::VULKAN:
#ifdef RENDERER_USE_VULKAN
            this->m_renderer = VulkanRenderer::CreateShared().As<Renderer>();
#else
            Logger::Error("Core::Init: Vulkan not available in this build");
#endif
    }
    
    this->m_renderer->Create(this->m_pWindow);
    
    this->m_input->SetWindow(this->m_pWindow);
    glfwSetKeyCallback(this->m_pWindow, Input::KeyCallback);
    glfwSetMouseButtonCallback(this->m_pWindow, Input::MouseButtonCallback);

    /*this->m_resMgr->SetRenderer(this->m_renderer);
    this->m_sceneMgr = SceneManager::GetInstance();
    this->m_sceneMgr->Start();*/
}

/* Our core update method */
void 
Core::Update() {
    /* While window should not close */
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        this->m_time->PreUpdate();
        glfwPollEvents(); // Poll GLFW events
        this->m_sceneMgr->Update();
        this->m_input->Close();
        this->m_time->PostUpdate();
    }
}

Core* 
Core::GetInstance() {
    if (Core::m_instance == nullptr)
        Core::m_instance = new Core();
    return Core::m_instance;
}
