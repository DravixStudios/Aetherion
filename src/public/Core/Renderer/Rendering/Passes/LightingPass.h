#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/Passes/GBufferPass.h"

class LightingPass : public BasePass {
public:
	struct Input {
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle orm;
		TextureHandle emissive;
		TextureHandle position;
		TextureHandle depth;
	};

	struct Output {
		TextureHandle lightingOutput;
	};

	void Init(Ref<Device> device) override;
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) override;

	void SetInput(const GBufferPass::Output& gbOutput);
	void SetLightData(
		Ref<DescriptorSet> lightSet, 
		Ref<GPUBuffer> vertexBuffer,
		Ref<GPUBuffer> indexBuffer,
		uint32_t nIndexCount
	);
	void SetGBufferDescriptorSet(Ref<DescriptorSet> gbSet);

	Output GetOutput() const { return this->m_output; }
private:
	Input m_input;
	Output m_output;
	Ref<DescriptorSet> m_gbufferSet;
	Ref<DescriptorSet> m_lightSet;

	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;
	uint32_t m_nIndexCount = 0;
};