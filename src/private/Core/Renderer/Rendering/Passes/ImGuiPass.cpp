#include "Core/Renderer/Rendering/Passes/ImGuiPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Fonts/RobotoRegular.h"

#include "Icons/Folder.h"
#include "Icons/Mesh.h"
#include "Icons/Scene.h"

#include <functional>
#include <nfd.h>

struct AssetBrowserState {
    Ref<ProjectTree::TreeNode> currentNode;
    Deque<Ref<ProjectTree::TreeNode>> history;
};

static AssetBrowserState s_browserState;

struct EditorIcons {
    Ref<GPUTexture> folderImage = nullptr;
    Ref<ImageView> folderView = nullptr;
    Ref<DescriptorSet> folderSet = nullptr;

    Ref<GPUTexture> meshImage = nullptr;
    Ref<ImageView> meshView = nullptr;
    Ref<DescriptorSet> meshSet = nullptr;

    Ref<GPUTexture> sceneImage = nullptr;
    Ref<ImageView> sceneView = nullptr;
    Ref<DescriptorSet> sceneSet = nullptr;
};

static EditorIcons s_icons;

/**
* ImGui pass initialization
* 
* @param device Logical device
*/
void
ImGuiPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* ImGui pass initialization
* 
* @param device Logical device
* @param nFramesInFlight Frames in flight count
*/
void 
ImGuiPass::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;
    this->m_sceneImGuiSets.resize(nFramesInFlight, Ref<DescriptorSet>());
}

/**
* ImGui pass node setup
* 
* @param builder Render graph builder
*/
void
ImGuiPass::SetupNode(RenderGraphBuilder& builder) {
	builder.UseColorOutput(this->m_output, EImageLayout::PRESENT_SRC);
	builder.SetDimensions(this->m_nWidth, this->m_nHeight);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight));
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
}

/**
* Executes the ImGui pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
* @param nFramesInFlight Frames in flight count
*/
void
ImGuiPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nImgIdx) {
	this->m_imgui->NewFrame();

    float hierarchyPadding = 50.f;
    float hierarchyHeight = static_cast<float>(this->m_nHeight) - (hierarchyPadding * 2);

    ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(pViewport->WorkPos);
    ImGui::SetNextWindowSize(pViewport->WorkSize);
    ImGui::SetNextWindowViewport(pViewport->ID);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockSpaceWindow", nullptr, windowFlags);
    ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::Begin("Viewport");
    ImVec2 actualSize = ImGui::GetContentRegionAvail();
    if (actualSize.x != this->m_viewportSize.x || actualSize.y != this->m_viewportSize.y) {
        this->m_viewportSize = actualSize;
        this->m_bPendingResize = true;
        this->m_pendingSize = actualSize;
    }

    this->m_imgui->Image(this->m_sceneImGuiSets[nImgIdx], actualSize);
    ImGui::End();

    ImGui::Begin("Hierarchy");
    ImGui::End();

    this->ShowAssetBrowser();

    ImGui::Begin("Sun Debug");
    if (ImGui::DragFloat("Sun Rotation X", &this->m_sunRotation.x, .1f)) {
        this->m_bSunChanged = true;
    }
    if (ImGui::DragFloat("Sun Rotation Y", &this->m_sunRotation.y, .1f)) {
        this->m_bSunChanged = true;
    }
    if (ImGui::DragFloat("Sun Rotation Z", &this->m_sunRotation.z, .1f)) {
        this->m_bSunChanged = true;
    }
    ImGui::End();

    /* Main menu bar */
    if (ImGui::BeginMainMenuBar()) {
	    if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open project...")) {
                /* Open a dialog for selecting the project */
                nfdu8char_t* pOutPath;
                nfdu8filteritem_t filters[1] = { { "Aetherion Project", "aethproj" } };
                nfdopendialogu8args_t args = {  };
                args.filterList = filters;
                args.filterCount = 1;

                nfdresult_t result = NFD_OpenDialogU8_With(&pOutPath, &args);
                
                /* Get project manager instance */
                ProjectManager* projMgr = ProjectManager::GetInstance();

                switch (result) {
                case NFD_OKAY:
                    projMgr->OpenProject(pOutPath);

                    s_browserState = { };
                    s_browserState.currentNode = projMgr->GetProjectTree().root;
                    break;
                default:
                    Logger::Error("ImGuiPass::Execute: Failed selecting project directory: {}", NFD_GetError());
                    break;
                }
            }
            
            ImGui::EndMenu();
	    }

	    if (ImGui::MenuItem("Assets")) {

	    }

	    if (ImGui::MenuItem("GameObject")) {

	    }

	    ImGui::EndMainMenuBar();
    }

	
	this->m_imgui->Render(context);
}

/**
* Show asset browser
* 
* @param node Asset browser node
*/
void 
ImGuiPass::ShowAssetBrowser() {
    ImGui::Begin("Project");

    Ref<ProjectTree::TreeNode> node = s_browserState.currentNode;
    if (!node) {
        ImGui::Text("No folder selected");
        ImGui::End();
        return;
    }

    /* Breadcrumb system */
    Vector<Ref<ProjectTree::TreeNode>> breadcrumb;

    {
        Ref<ProjectTree::TreeNode> n = s_browserState.currentNode;
        while (n) {
            breadcrumb.push_back(n);
            Ref<ProjectTree::TreeNode> parent = n->parent.lock();

            if (!parent) break;
            n = parent;
        }

        std::reverse(breadcrumb.begin(), breadcrumb.end());
    }

    for (uint32_t i = 0; i < breadcrumb.size(); ++i) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled(">");
            ImGui::SameLine();
        }

        String name = fs::path(breadcrumb[i]->dir.name).filename().string();

        if (i == breadcrumb.size() - 1) {
            ImGui::Text("%s", name.c_str());
        }
        else {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::SmallButton(name.c_str())) {
                s_browserState.history.clear();
                for (uint32_t j = 0; j < i; ++j) {
                    s_browserState.history.push_back(breadcrumb[j]);
                }
                s_browserState.currentNode = breadcrumb[i];
            }
            ImGui::PopID();
        }
    }

    ImGui::Separator();

    /* Asset browser elements */
    float cellSize = 96.f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int nColumnCount = static_cast<int>(panelWidth / cellSize);

    if (nColumnCount < 1) nColumnCount = 1;

    ImGui::Columns(nColumnCount, 0, false);

    for (auto& [id, child] : node->subNodes) {
        ImGui::PushID(id);
        String name = fs::path(child->dir.name).filename().string();

        float iconSize = cellSize - 20.f;
        float textWidth = ImGui::CalcTextSize(name.c_str()).x;
        float offsetX = (cellSize - iconSize) * .5f;

        if (this->m_imgui->ImageButton(s_icons.folderSet, name, ImVec2{ cellSize - 20, cellSize - 20 })) {
            s_browserState.history.push_back(node);
            s_browserState.currentNode = child;
        }

        ImGui::PopID();
        ImGui::NextColumn();
    }

    for (uint32_t i = 0; i < node->assets.size(); ++i) {
        const AssetHandle& asset = node->assets[i];
        EAssetType assetType = asset.type;

        switch (assetType) {
            case EAssetType::MESH:
            {
                Name name = ProjectManagerHelpers::GetAssetName(asset);
                String label = String(name);

                ImGui::PushID(i);

                if (this->m_imgui->ImageButton(s_icons.meshSet, label, ImVec2{ cellSize - 20, cellSize - 20 })) {
                    Logger::Debug("Clicked asset: {}", label);
                }

                ImGui::PopID();
                ImGui::NextColumn();
            }
                break;
            default:
                break;
        }

    }

    ImGui::Columns(1);

    ImGui::End();
}

void
ImGuiPass::Resize(uint32_t nWidth, uint32_t nHeight) {
	this->SetDimensions(nWidth, nHeight);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight));
}

/**
* Set imgui pass input
*
* @param input Scene texture handle
* @param transientPool Transient resource pool
*/
void
ImGuiPass::SetInput(TextureHandle input, TransientResourcePool& transientPool, uint32_t nImgIdx) {
    this->m_input = input;

    if (!this->m_pool) {
        this->CreateResources();
        this->SetupTheme();
    }

    /* Get image view from transient resource pool */
    Ref<ImageView> sceneView = transientPool.GetImageView(input);

    /* Clear last descriptor if exists */
    if (this->m_sceneImGuiSets[nImgIdx]) {
        this->m_imgui->RemoveTexture(this->m_sceneImGuiSets[nImgIdx]);
        this->m_sceneImGuiSets[nImgIdx] = Ref<DescriptorSet>();
    }

    this->m_sceneImGuiSets[nImgIdx] = this->m_imgui->AddTexture(this->m_sampler, sceneView, EImageLayout::SHADER_READ_ONLY);
}

/**
* Set imgui pass output
* 
* @param output Output texture handle
*/
void
ImGuiPass::SetOutput(TextureHandle output) {
	this->m_output = output;
}

/**
* Sets the imgui pass window
* 
* @param pWindow Pointer to GLFW window
*/
void 
ImGuiPass::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
}

/**
* Creates ImGui resources
*/
void
ImGuiPass::CreateResources() {
	/* Create descriptor pool */
	DescriptorPoolSize poolSize = { };
	poolSize.nDescriptorCount = static_cast<uint32_t>(IMGUI_DESCRIPTOR_POOL_SIZE);
	poolSize.type = EDescriptorType::COMBINED_IMAGE_SAMPLER;

	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = static_cast<uint32_t>(IMGUI_DESCRIPTOR_POOL_SIZE);
	poolInfo.poolSizes = Vector{ poolSize };
	
	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

    /* Create sampler */
    SamplerCreateInfo samplerInfo = { };
    samplerInfo.addressModeU = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.addressModeV = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.addressModeW = EAddressMode::CLAMP_TO_EDGE;
    samplerInfo.minFilter = EFilter::NEAREST;
    samplerInfo.magFilter = EFilter::NEAREST;
    samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_NEAREST;

    this->m_sampler = this->m_device->CreateSampler(samplerInfo);

	/* 
		Create compatible render pass 

		TODO: Select format from the swapchain
	*/
	AttachmentDescription attachment = { };
	attachment.format = GPUFormat::BGRA8_UNORM;
	attachment.initialLayout = EImageLayout::COLOR_ATTACHMENT;
	attachment.finalLayout = EImageLayout::PRESENT_SRC;
	attachment.loadOp = EAttachmentLoadOp::LOAD;
	attachment.storeOp = EAttachmentStoreOp::STORE;
	attachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	attachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
	attachment.sampleCount = ESampleCount::SAMPLE_1;

	SubpassDescription subpass = { };
	subpass.colorAttachments = { { 0, EImageLayout::COLOR_ATTACHMENT } };

	RenderPassCreateInfo rpInfo = { };
	rpInfo.attachments = Vector{ attachment };
	rpInfo.subpasses = Vector{ subpass };
    rpInfo.dependencies = GetDefaultSubpassDependencies(false);
	
	this->m_renderPass = this->m_device->CreateRenderPass(rpInfo);

    /* Create ImGui */
    ImGuiImplCreateInfo imguiInfo = { };
    imguiInfo.descriptorPool = this->m_pool;
    imguiInfo.nFramesInFlight = this->m_nFramesInFlight;
    imguiInfo.pWindow = this->m_pWindow;
    imguiInfo.renderPass = this->m_renderPass;

    this->m_imgui = this->m_device->CreateImGui(imguiInfo);

    /* Load icons */

    /*
        loadIcon lambda function

        This function rasterizes the
        SVG of our icon, loads it to
        the GPU and adds it to ImGuiImpl
    */
    std::function<void(
        const char*, 
        Ref<GPUTexture>&, 
        Ref<ImageView>&,
        Ref<DescriptorSet>&
    )> loadIcon = [this](
            const char* svg,
            Ref<GPUTexture>& outTexture,
            Ref<ImageView>& outView,
            Ref<DescriptorSet>& outSet
    ) {
        /* Load SVG */
        uint32_t nWidth = 0;
        uint32_t nHeight = 0;

        const Vector<unsigned char> data = LoadSVG(svg, 256.f, nWidth, nHeight);

        /* Create staging buffer */
        BufferCreateInfo buffInfo = { };
        buffInfo.type = EBufferType::STAGING_BUFFER;
        buffInfo.pcData = data.data();
        buffInfo.nSize = data.size();
        buffInfo.usage = EBufferUsage::TRANSFER_SRC;
        buffInfo.sharingMode = ESharingMode::EXCLUSIVE;

        Ref<GPUBuffer> staging = this->m_device->CreateBuffer(buffInfo);

        /* Create texture */
        TextureCreateInfo textureInfo = { };
        textureInfo.buffer = staging;
        textureInfo.extent.width = nWidth;
        textureInfo.extent.height = nHeight;
        textureInfo.extent.depth = 1;
        textureInfo.format = GPUFormat::RGBA8_UNORM;
        textureInfo.imageType = ETextureDimensions::TYPE_2D;
        textureInfo.initialLayout = ETextureLayout::UNDEFINED;
        textureInfo.nMipLevels = 1;
        textureInfo.nArrayLayers = 1;
        textureInfo.sharingMode = ESharingMode::EXCLUSIVE;
        textureInfo.samples = ESampleCount::SAMPLE_1;
        textureInfo.tiling = ETextureTiling::OPTIMAL;
        textureInfo.usage = ETextureUsage::SAMPLED | ETextureUsage::TRANSFER_DST;

        Ref<GPUTexture> texture = this->m_device->CreateTexture(textureInfo);

        /* Create image view */
        ImageViewCreateInfo viewInfo = { };
        viewInfo.image = texture;
        viewInfo.format = textureInfo.format;
        viewInfo.viewType = EImageViewType::TYPE_2D;
        viewInfo.subresourceRange.nBaseArrayLayer = 0;
        viewInfo.subresourceRange.nBaseMipLevel = 0;
        viewInfo.subresourceRange.nLayerCount = 1;
        viewInfo.subresourceRange.nLevelCount = 1;

        Ref<ImageView> imageView = this->m_device->CreateImageView(viewInfo);

        /* Add texture to ImGuiImpl */
        Ref<DescriptorSet> set = this->m_imgui->AddTexture(this->m_sampler, imageView, EImageLayout::SHADER_READ_ONLY);

        /* Output */
        outTexture = texture;
        outView = imageView;
        outSet = set;
    };

    s_icons = { };

    /* Folder Icon */
    loadIcon(FolderSVG, s_icons.folderImage, s_icons.folderView, s_icons.folderSet);

    /* 
        Mesh icon 
        
        TODO: Generate a preview of
        the mesh when uploading the mesh
    */
    loadIcon(MeshSVG, s_icons.meshImage, s_icons.meshView, s_icons.meshSet);

    /* Scene icon */
    loadIcon(SceneSVG, s_icons.sceneImage, s_icons.sceneView, s_icons.sceneSet);
}

/**
* ImGui theme setup
* 
* Used theme: Catppuchin mocha theme
* https://github.com/ocornut/imgui/issues/707#issuecomment-3592676777
*/
void
ImGuiPass::SetupTheme() {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig fontCfg = { };
    fontCfg.FontDataOwnedByAtlas = false;

    io.Fonts->AddFontFromMemoryTTF(Roboto_Regular_ttf, sizeof(Roboto_Regular_ttf), 16.f, &fontCfg);

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Catppuccin Mocha Palette
    // --------------------------------------------------------
    const ImVec4 base = ImVec4(0.117f, 0.117f, 0.172f, 1.0f); // #1e1e2e
    const ImVec4 mantle = ImVec4(0.109f, 0.109f, 0.156f, 1.0f); // #181825
    const ImVec4 surface0 = ImVec4(0.200f, 0.207f, 0.286f, 1.0f); // #313244
    const ImVec4 surface1 = ImVec4(0.247f, 0.254f, 0.337f, 1.0f); // #3f4056
    const ImVec4 surface2 = ImVec4(0.290f, 0.301f, 0.388f, 1.0f); // #4a4d63
    const ImVec4 overlay0 = ImVec4(0.396f, 0.403f, 0.486f, 1.0f); // #65677c
    const ImVec4 overlay2 = ImVec4(0.576f, 0.584f, 0.654f, 1.0f); // #9399b2
    const ImVec4 text = ImVec4(0.803f, 0.815f, 0.878f, 1.0f); // #cdd6f4
    const ImVec4 subtext0 = ImVec4(0.639f, 0.658f, 0.764f, 1.0f); // #a3a8c3
    const ImVec4 mauve = ImVec4(0.796f, 0.698f, 0.972f, 1.0f); // #cba6f7
    const ImVec4 peach = ImVec4(0.980f, 0.709f, 0.572f, 1.0f); // #fab387
    const ImVec4 yellow = ImVec4(0.980f, 0.913f, 0.596f, 1.0f); // #f9e2af
    const ImVec4 green = ImVec4(0.650f, 0.890f, 0.631f, 1.0f); // #a6e3a1
    const ImVec4 teal = ImVec4(0.580f, 0.886f, 0.819f, 1.0f); // #94e2d5
    const ImVec4 sapphire = ImVec4(0.458f, 0.784f, 0.878f, 1.0f); // #74c7ec
    const ImVec4 blue = ImVec4(0.533f, 0.698f, 0.976f, 1.0f); // #89b4fa
    const ImVec4 lavender = ImVec4(0.709f, 0.764f, 0.980f, 1.0f); // #b4befe

    // Main window and backgrounds
    colors[ImGuiCol_WindowBg] = base;
    colors[ImGuiCol_ChildBg] = base;
    colors[ImGuiCol_PopupBg] = surface0;
    colors[ImGuiCol_Border] = surface1;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = surface0;
    colors[ImGuiCol_FrameBgHovered] = surface1;
    colors[ImGuiCol_FrameBgActive] = surface2;
    colors[ImGuiCol_TitleBg] = mantle;
    colors[ImGuiCol_TitleBgActive] = surface0;
    colors[ImGuiCol_TitleBgCollapsed] = mantle;
    colors[ImGuiCol_MenuBarBg] = mantle;
    colors[ImGuiCol_ScrollbarBg] = surface0;
    colors[ImGuiCol_ScrollbarGrab] = surface2;
    colors[ImGuiCol_ScrollbarGrabHovered] = overlay0;
    colors[ImGuiCol_ScrollbarGrabActive] = overlay2;
    colors[ImGuiCol_CheckMark] = green;
    colors[ImGuiCol_SliderGrab] = sapphire;
    colors[ImGuiCol_SliderGrabActive] = blue;
    colors[ImGuiCol_Button] = surface0;
    colors[ImGuiCol_ButtonHovered] = surface1;
    colors[ImGuiCol_ButtonActive] = surface2;
    colors[ImGuiCol_Header] = surface0;
    colors[ImGuiCol_HeaderHovered] = surface1;
    colors[ImGuiCol_HeaderActive] = surface2;
    colors[ImGuiCol_Separator] = surface1;
    colors[ImGuiCol_SeparatorHovered] = mauve;
    colors[ImGuiCol_SeparatorActive] = mauve;
    colors[ImGuiCol_ResizeGrip] = surface2;
    colors[ImGuiCol_ResizeGripHovered] = mauve;
    colors[ImGuiCol_ResizeGripActive] = mauve;
    colors[ImGuiCol_Tab] = surface0;
    colors[ImGuiCol_TabHovered] = surface2;
    colors[ImGuiCol_TabActive] = surface1;
    colors[ImGuiCol_TabUnfocused] = surface0;
    colors[ImGuiCol_TabUnfocusedActive] = surface1;
    colors[ImGuiCol_PlotLines] = blue;
    colors[ImGuiCol_PlotLinesHovered] = peach;
    colors[ImGuiCol_PlotHistogram] = teal;
    colors[ImGuiCol_PlotHistogramHovered] = green;
    colors[ImGuiCol_TableHeaderBg] = surface0;
    colors[ImGuiCol_TableBorderStrong] = surface1;
    colors[ImGuiCol_TableBorderLight] = surface0;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = surface2;
    colors[ImGuiCol_DragDropTarget] = yellow;
    colors[ImGuiCol_NavHighlight] = lavender;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = subtext0;

    // Rounded corners
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Padding and spacing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 3.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.IndentSpacing = 21.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
}