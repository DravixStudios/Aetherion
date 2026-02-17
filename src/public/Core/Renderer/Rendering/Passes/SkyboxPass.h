#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"

class SkyboxPass : public BasePass {
public:
	struct Input {
		TextureHandle depth;
		TextureHandle hdrOutput;
	};

	struct SkyboxCamera {
		glm::mat4 inverseView;
		glm::mat4 inverseProj;
		glm::vec3 camPos;
	};

	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);
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

	Ref<GPURingBuffer> m_camBuff;

	Ref<DescriptorSet> m_camSet;
	Ref<DescriptorSetLayout> m_camSetLayout;
	Ref<DescriptorPool> m_camPool;

	void CreateDescriptors();
	void CreatePipeline();

	glm::mat4 m_view;
	glm::mat4 m_proj;
	glm::vec3 m_cameraPosition;

	uint32_t m_nFramesInFlight;

public:
	void UpdateCamera(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
		this->m_view = view;
		this->m_proj = proj;
		this->m_cameraPosition = cameraPos;
	}
};