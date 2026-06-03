#include "Core/Core.h"
#include "Core/Scene/SceneManager.h"
#include "Core/Renderer/ResourceManager.h"
#include "Core/Project/ProjectManager.h"

#include <nfd.h>

Core* Core::m_instance;

/* Drop callback for GLFW */
void
DropCallback(GLFWwindow* window, int nCount, const char** paths) {
    for (uint32_t i = 0; i < nCount; i++) {
        const char* path = paths[i];

        ProjectManager* projMgr = ProjectManager::GetInstance();

        if (!projMgr->ProjectLoaded()) {
            return;
        }

        Directory assetsDir = projMgr->GetAssetsDir();
        AssetManager::GetInstance()->ImportAsset(path, assetsDir.name);
    }
}

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

   
    this->m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Aetherion Engine", nullptr, nullptr); // GLFW Window creation
    glfwSetWindowUserPointer(this->m_pWindow, this);

    glfwSetFramebufferSizeCallback(this->m_pWindow, FramebufferSizeCallback);

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

    /* Create graphics context */
    this->m_contexts.resize(this->m_nImageCount);
    for (uint32_t i = 0; i < this->m_nImageCount; i++) {
        this->m_contexts[i] = this->m_device->CreateContext(this->m_pool);
    }
    
    this->CreateSwapchain();
    this->CreateSyncObjects();

    this->m_input->SetWindow(this->m_pWindow);
    glfwSetKeyCallback(this->m_pWindow, Input::KeyCallback);
    glfwSetDropCallback(this->m_pWindow, DropCallback);
    glfwSetMouseButtonCallback(this->m_pWindow, Input::MouseButtonCallback);

    this->m_deferredRenderer.Init(this->m_device, this->m_swapchain, this->m_nImageCount, this->m_pWindow);

    this->m_time = Time::GetInstance();

    this->m_sceneMgr = SceneManager::GetInstance();
    this->m_sceneMgr->Start();
    this->m_sceneMgr->SetDimensions(WIDTH, HEIGHT);

    glfwSetWindowTitle(this->m_pWindow, "No active project - Aetherion");

    ProjectManager::GetInstance()->SetOnProjectOpenedCallback(
        [this](const Project::Asset& projectAsset) {
            String title = projectAsset.name + " - Aetherion Engine";
            glfwSetWindowTitle(this->m_pWindow, title.c_str());

            String editorScene = projectAsset.editorScene;
            String runtimeScene = projectAsset.runtimeScene;

            /* TODO: Switch between editor and runtime scenes */
            AssetHandle sceneHandle = AssetHandle::FromPath(editorScene, EAssetType::SCENE);

            AssetVariant sceneAssetVariant = AssetManager::GetInstance()->GetAsset(sceneHandle);
            
            /* Check if asset variant holds SceneAsset */
            if (SceneAsset* pSceneAsset = std::get_if<SceneAsset>(&sceneAssetVariant)) {

            }
            else {
                Logger::Error("Core::Init:[OnProjectOpenedCallback]: Not a SceneAsset");
                return;
            }

        }
    );

    this->m_deferredRenderer.SetOnSceneSaveCallback([this]() {
        /* Get project manager */
        ProjectManager* projectMgr = ProjectManager::GetInstance();

        /* Check if we have any project loaded */
        if (!projectMgr->ProjectLoaded()) {
            Logger::Error("Core::Init:[OnSceneSaveCallback]: No project open");
            return;
        }

        /* Get current scene and project tree */
        Scene* scene = this->m_sceneMgr->GetCurrentScene();
        ProjectTree projectTree = projectMgr->GetProjectTree();
        Directory projectDir = projectTree.root->dir;

        /*
            Open a NFD Dialog

            TODO: Move this part to a separate class
            or system
        */
        nfdu8char_t* outPath;
        nfdu8filteritem_t filters[1] = { { "Aetherion Scene", "aeth" } };

        fs::path projectPath = projectDir.name;
        String sceneName = scene->GetName();
        String sceneFile = sceneName + ".aeth";

        nfdresult_t result = NFD_SaveDialogU8(
            &outPath,
            filters,
            1,
            reinterpret_cast<const nfdu8char_t*>(projectPath.u8string().c_str()),
            reinterpret_cast<const nfdu8char_t*>(sceneFile.c_str())
        );

        /* Check NFD result */
        switch (result) {
            case NFD_OKAY:
                Logger::Debug("Core::Init:[OnSceneSaveCallback]: Saving scene. {}", outPath);
                break;
            case NFD_CANCEL:
                Logger::Debug("Core::Init:[OnSceneSaveCallback]: User cancelled dialog");
                return;
            case NFD_ERROR:
            {
                const char* error = NFD_GetError();
                Logger::Error("Core::Init:[OnSceneSaveCallback]: {}", error);
                return;
            }
        }

        String scenePath = outPath;

        /* 
            Serialize scene and save it

            TODO: Check that file is correctly saved
        */
        SceneAsset sceneAsset = scene->SerializeScene();

        AssetManager* assetMgr = AssetManager::GetInstance();

        if(!assetMgr->SaveScene(scenePath, sceneAsset)) {
            Logger::Error("Core::Init:[OnSceneSaveCallback]: Failed saving scene {}", scenePath);
            return;
        }

        Logger::Info("Core::Init:[OnSceneSaveCallback]: Scene {} saved at {}", sceneName, scenePath);
    });

    this->m_deferredRenderer.SetOnDropToViewportCallback([](const AssetHandle& handle) {
        /* Get required managers */
        AssetManager* assetMgr = AssetManager::GetInstance();
        SceneManager* sceneMgr = SceneManager::GetInstance();

        Logger::Debug("Core::Init:[OnDropToViewportCallback]: Dropped asset: {} to viewport", handle.uuid);

        EAssetType assetType = handle.type;

        switch (assetType) {
            case EAssetType::MESH: 
            {
                /* 
                * If mesh asset dropped to viewport,
                * create a new gameobject with a MeshComponent 
                */
                GameObject* pObj = new GameObject(std::to_string(handle.uuid));
                Mesh* pMesh = new Mesh("MeshComponent");
                pMesh->LoadAsset(handle);
                
                pObj->AddComponent("MeshComponent", pMesh);

                Scene* currentScene = sceneMgr->GetCurrentScene();
                
                //pObj->transform.scale = Vector3(.1f, .1f, .1f);
                currentScene->AddObject(pObj);

                break;
            }
            default:
                break;
        }
    });
}

/* Our core update method */
void 
Core::Update() {
    /* While window should not close */
    while (!glfwWindowShouldClose(this->m_pWindow)) {
        this->m_time->PreUpdate();

        /* Manage window resizing */
        if (this->m_bWindowResized) {
            int nWidth = 0;
            int nHeight = 0;

            glfwGetFramebufferSize(this->m_pWindow, &nWidth, &nHeight);

            /* If window is minimized, wait till has dimensions again */
            while (nWidth == 0 || nHeight == 0) {
                glfwGetFramebufferSize(this->m_pWindow, &nWidth, &nHeight);
                glfwWaitEvents();
            }

            this->m_device->WaitIdle();

            this->m_deferredRenderer.Invalidate();
            this->m_swapchain->Rebuild(static_cast<uint32_t>(nWidth), static_cast<uint32_t>(nHeight));
            this->m_deferredRenderer.Resize(nWidth, nHeight);

            SceneManager::GetInstance()->SetDimensions(nWidth, nHeight);

            this->m_bWindowResized = false;
        }

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
            auto it = components.find("MeshComponent");
            if (it != components.end()) {
                Mesh* mesh = dynamic_cast<Mesh*>(it->second);
                if (mesh && mesh->IsLoaded()) {
                    this->m_deferredRenderer.UploadMesh(mesh->GetMeshData());

                    mesh->ClearTextureData();
                }
            }
        }

        this->m_deferredRenderer.FinalizeMeshUploads();

        const auto& meshCache = this->m_deferredRenderer.GetUploadedMeshes();
        this->m_sceneCollector.SetUploadedMeshes(&meshCache);
        CollectedDrawData drawData = this->m_sceneCollector.Collect(currentScene);

        this->m_deferredRenderer.Render(context, this->m_swapchain, drawData, nImgIdx);

        commandBuffer->End();

        SubmitInfo submitInfo = { };
        submitInfo.commandBuffers = { commandBuffer };
        submitInfo.signalSemaphores = { this->m_renderFinishedSemaphores[this->m_nImageIndex] };
        submitInfo.waitSemaphores = { this->m_imageAvailableSemaphores[this->m_nImageIndex] };
        submitInfo.waitStages = { EPipelineStage::COLOR_ATTACHMENT_OUTPUT };

        this->m_device->Submit(submitInfo, this->m_inFlightFences[this->m_nImageIndex]);

        this->m_swapchain->Present(nImgIdx, Vector{ this->m_renderFinishedSemaphores[this->m_nImageIndex] });

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

        this->m_imageAvailableSemaphores[i] = imageAvailableSemaphore;
        this->m_renderFinishedSemaphores[i] = renderSemaphore;
        this->m_inFlightFences[i] = inFlightFence;
    }


}