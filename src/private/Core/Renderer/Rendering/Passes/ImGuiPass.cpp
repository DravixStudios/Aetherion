#include "Core/Renderer/Rendering/Passes/ImGuiPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"

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

	ImGui::Begin("Hierarchy");
	ImGui::Button("Test");
	ImGui::End();

	this->m_imgui->Render(context);
}

void
ImGuiPass::Resize(uint32_t nWidth, uint32_t nHeight) {
	this->SetDimensions(nWidth, nHeight);
}

void
ImGuiPass::SetOutput(TextureHandle output) {
	this->m_output = output;
	if (!this->m_pool) {
		this->CreateResources();
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