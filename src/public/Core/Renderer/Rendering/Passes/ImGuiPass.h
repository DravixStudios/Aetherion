#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GraphNode.h"
#include "Core/Renderer/ImGuiImpl.h"

#include <imgui/imgui.h>

#define IMGUI_DESCRIPTOR_POOL_SIZE 8

class ImGuiPass : public BasePass {
public:
	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);

	void SetupNode(RenderGraphBuilder& builder) override;

	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex = 0) override;
	void Resize(uint32_t nWidth, uint32_t nHeight);

	void SetOutput(TextureHandle output);
	void SetWindow(GLFWwindow* pWindow);

private:
	void CreateResources();
	void SetupTheme();

	TextureHandle m_output;

	uint32_t nFramesInFlight = 0;

	Ref<DescriptorPool> m_pool;
	Ref<RenderPass> m_renderPass;
	Ref<ImGuiImpl> m_imgui;

	GLFWwindow* m_pWindow = nullptr;
};