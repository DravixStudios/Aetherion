#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GraphNode.h"

/**
* Tonemap pass
*/
class TonemapPass : public BasePass {
public:
	virtual void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, Ref<Swapchain> swapchain, uint32_t nFramesInFlight);

	virtual void SetupNode(RenderGraphBuilder& builder) override;

	virtual void Execute(
		Ref<GraphicsContext> context,
		RenderGraphContext& graphCtx,
		uint32_t nFrameIndex = 0
	) override;

	/**
	* Sets the input HDR image
	* 
	* @param input Input HDR image
	*/
	void SetInput(TextureHandle input);

	/**
	* Sets the output image (Backbuffer)
	* 
	* @param output Output image
	*/
	void SetOutput(TextureHandle output);

	/**
	* Sets the screen quad data
	* 
	* @param vertexBuffer Vertex buffer
	* @param indexBuffer Index buffer
	* @param nIndexCount Index count
	*/
	void SetScreenQuad(Ref<GPUBuffer> vertexBuffer, Ref<GPUBuffer> indexBuffer, uint32_t nIndexCount);

private:
	void CreatePipeline(GPUFormat format);
	void CreateDescriptorSets();
	void UpdateDescriptorSet(uint32_t nFrameIndex, Ref<ImageView> inputView);

	TextureHandle m_input;
	TextureHandle m_output;

	Ref<DescriptorSetLayout> m_setLayout;
	Ref<DescriptorPool> m_pool;
	Vector<Ref<DescriptorSet>> m_sets;
	uint32_t m_nFramesInFlight = 0;

	Ref<Sampler> m_sampler; 

	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;
	uint32_t m_nIndexCount = 0;

	Ref<RenderPass> m_compatRenderPass;
};
