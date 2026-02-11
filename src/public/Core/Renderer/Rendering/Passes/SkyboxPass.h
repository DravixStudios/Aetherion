#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"

class SkyboxPass : public BasePass {
public:
	struct Input {
		TextureHandle depth;
		TextureHandle hdrOutput;
	};

	void Init(Ref<Device>) override;
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIndex = 0) override;

	void SetInput(const Input& input) { this->m_input = input; }

	void SetSkyboxData(
		Ref<DescriptorSet> skyboxSet,
		Ref<DescriptorSetLayout> skyboxSetLayout,
		Ref<GPUBuffer> vertexBuffer, 
		Ref<GPUBuffer> indexBuffer,
		uint32_t nIndexCount
	);

	Ref<PipelineLayout> GetPipelineLayout() const { return this->m_pipelineLayout; }

private:
	Input m_input;
	
	Ref<Device> m_device;
	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;

	Ref<Pipeline> m_pipeline;
	Ref<PipelineLayout> m_pipelineLayout;

	Ref<RenderPass> m_compatRenderPass;

	uint32_t m_nIndexCount = 0;

	Ref<DescriptorSet> m_skyboxSet;
	Ref<DescriptorSetLayout> m_skyboxSetLayout;

	void CreatePipeline();

	glm::mat4 m_view;
	glm::mat4 m_proj;
	glm::vec3 m_cameraPosition;

public:
	void UpdateCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
		this->m_view = view;
		this->m_proj = proj;
		this->m_cameraPosition = cameraPos;
	}
};