#include "Core/Renderer/Vulkan/VulkanImGuiImpl.h"

VulkanImGuiImpl::VulkanImGuiImpl(Ref<VulkanDevice> device) 
	: m_device(device) {}

/**
* Creates Vulkan ImGUI implementation
* 
* @param createInfo ImGUI implementation create info
*/
void
VulkanImGuiImpl::Create(const ImGuiImplCreateInfo& createInfo) {
	this->m_pWindow = createInfo.pWindow;
	this->m_pool = createInfo.descriptorPool;

	ImGui_ImplVulkan_InitInfo initInfo = { };
	initInfo.ApiVersion = VK_API_VERSION_1_3;
	initInfo.Instance = this->m_device->GetVkInstance();
	initInfo.PhysicalDevice = this->m_device->GetVkPhysicalDevice();
	initInfo.Device = this->m_device->GetVkDevice();
	initInfo.Queue = this->m_device->GetGraphicsQueue();
	initInfo.QueueFamily = this->m_device->GetGraphicsQueueFamily();
	initInfo.MinImageCount = createInfo.nFramesInFlight;
	initInfo.ImageCount = createInfo.nFramesInFlight;
	initInfo.DescriptorPool = createInfo.descriptorPool.As<VulkanDescriptorPool>()->GetVkPool();
	initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.PipelineInfoMain.Subpass = 0;
	initInfo.PipelineInfoMain.RenderPass = createInfo.renderPass.As<VulkanRenderPass>()->GetVkRenderPass();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(this->m_pWindow, true);
	ImGui_ImplVulkan_Init(&initInfo);
}

/**
* Starts a new ImGUI frame
*/
void
VulkanImGuiImpl::NewFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

/**
* Renders the GUI
*/
void
VulkanImGuiImpl::Render(Ref<GraphicsContext> context) {
	Ref<CommandBuffer> commandBuff = context->GetCommandBuffer();

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(
		ImGui::GetDrawData(),
		commandBuff.As<VulkanCommandBuffer>()->GetVkCommandBuffer()
	);
}