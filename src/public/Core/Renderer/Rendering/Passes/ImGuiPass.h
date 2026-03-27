#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GraphNode.h"
#include "Core/Renderer/ImGuiImpl.h"
#include "Core/Project/ProjectManager.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#define IMGUI_DESCRIPTOR_POOL_SIZE 64

class ImGuiPass : public BasePass {
public:
	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);

	void SetupNode(RenderGraphBuilder& builder) override;

	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nImgIdx = 0) override;
	void Resize(uint32_t nWidth, uint32_t nHeight);

	bool HasPendingResize() const { return this->m_bPendingResize; }
	ImVec2 GetPendingSize() const { return this->m_pendingSize; }
	void ClearPendingResize() { this->m_bPendingResize = false; }

	glm::vec3 GetSunRotation() const { return this->m_sunRotation; }
	bool SunChanged() { return this->m_bSunChanged; }
	void NotifySunUpdated() { this->m_bSunChanged = false; }

	void SetInput(TextureHandle input, TransientResourcePool& transientPool, uint32_t nImgIdx);
	void SetOutput(TextureHandle output);
	void SetWindow(GLFWwindow* pWindow);

private:
	void ShowAssetBrowser();

	void CreateResources();
	void SetupTheme();

	TextureHandle m_input;
	TextureHandle m_output;

	uint32_t m_nFramesInFlight = 0;

	glm::vec3 m_sunRotation = glm::vec3(70.f, 70.f, 0.f);
	bool m_bSunChanged = true; // True by default for first calculations

	Ref<DescriptorPool> m_pool;
	Ref<RenderPass> m_renderPass;
	Ref<ImGuiImpl> m_imgui;
	Ref<Sampler> m_sampler;
	Vector<Ref<DescriptorSet>> m_sceneImGuiSets;

	ImVec2 m_viewportSize;
	bool m_bPendingResize = false;
	ImVec2 m_pendingSize = { 0, 0 };

	GLFWwindow* m_pWindow = nullptr;
};