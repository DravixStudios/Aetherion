#include "Core/Renderer/Rendering/Passes/ImGuiPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Fonts/RobotoRegular.h"

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
	this->nFramesInFlight = nFramesInFlight;
}

/**
* ImGui pass node setup
* 
* @param builder Render graph builder
*/
void
ImGuiPass::SetupNode(RenderGraphBuilder& builder) {
	builder.UseColorOutput(this->m_output, EImageLayout::PRESENT_SRC, EAttachmentLoadOp::LOAD);
	builder.SetDimensions(this->m_nWidth, this->m_nHeight);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight));
}

/**
* Executes the ImGui pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
* @param nFramesInFlight Frames in flight count
*/
void
ImGuiPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFramesInFlight) {
	this->m_imgui->NewFrame();

    float hierarchyPadding = 50.f;
    float hierarchyHeight = static_cast<float>(this->m_nHeight) - (hierarchyPadding * 2);

    ImGui::SetNextWindowPos(ImVec2{ hierarchyPadding, hierarchyPadding });
    ImGui::SetNextWindowSize(ImVec2{ 200.f, hierarchyHeight });
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoResize);

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

    ImGui::End();

	ImGui::BeginMainMenuBar();

	if (ImGui::MenuItem("File")) {

	}

	if (ImGui::MenuItem("Assets")) {

	}

	if (ImGui::MenuItem("GameObject")) {

	}

	ImGui::EndMainMenuBar();
	
	this->m_imgui->Render(context);
}

void
ImGuiPass::Resize(uint32_t nWidth, uint32_t nHeight) {
	this->SetDimensions(nWidth, nHeight);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight));
}

void
ImGuiPass::SetOutput(TextureHandle output) {
	this->m_output = output;
	if (!this->m_pool) {
		this->CreateResources();
		this->SetupTheme();
	}
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
	
	this->m_renderPass = this->m_device->CreateRenderPass(rpInfo);

	/* Create ImGui */
	ImGuiImplCreateInfo imguiInfo = { };
	imguiInfo.descriptorPool = this->m_pool;
	imguiInfo.nFramesInFlight = this->nFramesInFlight;
	imguiInfo.pWindow = this->m_pWindow;
	imguiInfo.renderPass = this->m_renderPass;
	
	this->m_imgui = this->m_device->CreateImGui(imguiInfo);
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