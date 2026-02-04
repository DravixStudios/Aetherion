#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/Shader.h"

class GBufferPass : public BasePass {
public:
	struct Output {
		TextureHandle albedo;
		TextureHandle normal;
		TextureHandle orm;
		TextureHandle emissive;
		TextureHandle position;
		TextureHandle depth;
	};

	void Init(Ref<Device> device) override;
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) override;
	
	void SetSceneData(
		Ref<DescriptorSet> sceneSet,
		Ref<DescriptorSetLayout> sceneSetLayout,
		Ref<DescriptorSet> bindlessSet,
		Ref<DescriptorSetLayout> bindlessSetLayout,
		Ref<GPUBuffer> vertexBuffer,
		Ref<GPUBuffer> indexBuffer,
		uint32_t nIndexCount
	);

	Output GetOutput() const { return this->m_output; }
private:
	Ref<RenderPass> m_compatRenderPass;

	Output m_output;
	Ref<DescriptorSet> m_sceneSet;
	Ref<DescriptorSetLayout> m_sceneSetLayout;

	Ref<DescriptorSet> m_bindlessSet;
	Ref<DescriptorSetLayout> m_bindlessSetLayout;

	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;
	uint32_t m_nIndexCount = 0;

	void CreatePipeline();
};