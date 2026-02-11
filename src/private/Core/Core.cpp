#include "Core/Core.h"
#include "Core/Scene/SceneManager.h"
#include "Core/Renderer/ResourceManager.h"

Core* Core::m_instance;

/* Core constructor */
Core::Core()
    : m_renderBackend(ERenderBackend::VULKAN), 
    m_resMgr(ResourceManager::GetInstance()), m_input(Input::GetInstance()),
    m_sampleCount(ESampleCount::SAMPLE_8), m_nImageCount(3) {}

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
    */
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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
    
    /* Create renderer */
    this->m_renderer->Create(this->m_pWindow);

    /* Create logical device */
    this->m_device = this->m_renderer->CreateDevice();

    /* Create a command pool for our Graphics context */
    CommandPoolCreateInfo poolInfo = { };
    poolInfo.flags = ECommandPoolFlags::RESET_COMMAND_BUFFER;
    poolInfo.nQueueFamilyIndex = 0; // 0 because Device will resolve it

    /* Create command pool */
    this->m_pool = this->m_device->CreateCommandPool(poolInfo, EQueueType::GRAPHICS);

    /* Create graphcics context */
    this->m_contexts.resize(this->m_nImageCount);
    for (uint32_t i = 0; i < this->m_nImageCount; i++) {
        this->m_contexts[i] = this->m_device->CreateContext(this->m_pool);
    }
    
    this->CreateSwapchain();
    this->CreateSyncObjects();

    this->m_input->SetWindow(this->m_pWindow);
    glfwSetKeyCallback(this->m_pWindow, Input::KeyCallback);
    glfwSetMouseButtonCallback(this->m_pWindow, Input::MouseButtonCallback);

    this->m_deferredRenderer.Init(this->m_device, this->m_swapchain, this->m_nImageCount);

    this->m_time = Time::GetInstance();

    this->m_sceneMgr = SceneManager::GetInstance();
    this->m_sceneMgr->Start();
}

/* Our core update method */
void 
Core::Update() {
    /* While window should not close */
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        this->m_device->WaitForFence(this->m_inFlightFences[this->m_nImageIndex]);
        this->m_inFlightFences[this->m_nImageIndex]->Reset();

        uint32_t nImgIdx = this->m_swapchain->AcquireNextImage(
            UINT64_MAX, 
            this->m_imageAvailableSemaphores[this->m_nImageIndex],
            Ref<Fence>()
        );

        Ref<GraphicsContext> context = this->m_contexts[this->m_nImageIndex];
        Ref<CommandBuffer> commandBuffer = context->GetCommandBuffer();
        commandBuffer->Reset();
        commandBuffer->Begin();

        Scene* currentScene = this->m_sceneMgr->GetCurrentScene();

        for (auto& [name, gameObject] : currentScene->GetObjects()) {
            auto components = gameObject->GetComponents();
            auto it = components.find("Mesh");
            if (it != components.end()) {
                Mesh* mesh = dynamic_cast<Mesh*>(it->second);
                if (mesh && mesh->IsLoaded()) {
                    this->m_deferredRenderer.UploadMesh(mesh->GetMeshData());
                }
            }
        }

        const std::map<String, UploadedMesh> meshCache = this->m_deferredRenderer.GetUploadedMeshes();
        this->m_sceneCollector.SetUploadedMeshes(&meshCache);
        CollectedDrawData drawData = this->m_sceneCollector.Collect(currentScene);

        this->m_deferredRenderer.Render(context, this->m_swapchain, drawData, nImgIdx);

        /* TODO: Render graph controlling this */
        context->ImageBarrier(
            this->m_swapchain->GetImage(nImgIdx),
            EImageLayout::UNDEFINED,
            EImageLayout::PRESENT_SRC
        );

        commandBuffer->End();

        SubmitInfo submitInfo = { };
        submitInfo.commandBuffers = { commandBuffer };
        submitInfo.signalSemaphores = { this->m_renderFinishedSemaphores[this->m_nImageIndex] };
        submitInfo.waitSemaphores = { this->m_imageAvailableSemaphores[this->m_nImageIndex] };
        submitInfo.waitStages = { EPipelineStage::COLOR_ATTACHMENT_OUTPUT };

        this->m_device->Submit(submitInfo, this->m_inFlightFences[this->m_nImageIndex]);

        this->m_swapchain->Present(nImgIdx, Vector{ this->m_renderFinishedSemaphores[this->m_nImageIndex] });

        this->m_time->PreUpdate();
        glfwPollEvents(); // Poll GLFW events
        this->m_sceneMgr->Update();
        this->m_input->Close();
        this->m_time->PostUpdate();
        this->m_nImageIndex = (this->m_nImageIndex + 1) % this->m_nImageCount;
    }

    this->m_device->WaitIdle();
}

Core* 
Core::GetInstance() {
    if (Core::m_instance == nullptr)
        Core::m_instance = new Core();
    return Core::m_instance;
}

/**
* Creates swapchain
*/
void
Core::CreateSwapchain() {
    /* TODO: Image count user-selectable */
    SwapchainCreateInfo scInfo = { };
    scInfo.nImageCount = this->m_nImageCount;
    scInfo.pWindow = this->m_pWindow;
    scInfo.width = WIDTH;
    scInfo.height = HEIGHT;
    scInfo.bEnableDepthStencil = true;

    this->m_swapchain = this->m_device->CreateSwapchain(scInfo);
}

/**
* Creates synchronization objects
*/
void 
Core::CreateSyncObjects() {
    this->m_imageAvailableSemaphores.resize(this->m_nImageCount);
    this->m_renderFinishedSemaphores.resize(this->m_nImageCount);
    this->m_inFlightFences.resize(this->m_nImageCount);

    FenceCreateInfo fenceInfo = { };
    fenceInfo.flags = EFenceFlags::SIGNALED;

    for (uint32_t i = 0; i < this->m_nImageCount; i++) {
        Ref<Semaphore> renderSemaphore = this->m_device->CreateSemaphore();
        Ref<Semaphore> imageAvailableSemaphore = this->m_device->CreateSemaphore();
        Ref<Fence> inFlightFence = this->m_device->CreateFence(fenceInfo);

        this->m_imageAvailableSemaphores[i] = renderSemaphore;
        this->m_renderFinishedSemaphores[i] = imageAvailableSemaphore;
        this->m_inFlightFences[i] = inFlightFence;
    }


}