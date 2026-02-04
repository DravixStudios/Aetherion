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
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) override;

	void SetInput(const Input& input) { this->m_input = input; }

	void SetSkyboxData(Ref<DescriptorSet> skyboxSet, Ref<GPUBuffer> cubeVb, uint32_t nVertexCount);
private:
	Input m_input;
	
	Ref<Device> m_device;
	Ref<GPUBuffer> m_cubeVb;

	uint32_t m_nVertexCount = 0;
};