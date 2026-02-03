#include "Core/Renderer/Rendering/Passes/GBufferPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Core/Renderer/Shader.h"

/**
* G-Buffer pass initialization
* @param device Logical device
*/
void
GBufferPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Setup the node
* 
* @param builder Render graph builder
*/
void
GBufferPass::SetupNode(RenderGraphBuilder& builder) {
	TextureDesc colorDesc = { };
	colorDesc.nWidth = this->m_nWidth;
	colorDesc.nHeight = this->m_nHeight;
	colorDesc.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;

	/* Albedo */
	colorDesc.format = GBufferLayout::ALBEDO;
	this->m_output.albedo = builder.CreateColorOutput(colorDesc);

	/* Normal */
	colorDesc.format = GBufferLayout::NORMAL;
	this->m_output.normal = builder.CreateColorOutput(colorDesc);

	/* ORM */
	colorDesc.format = GBufferLayout::ORM;
	this->m_output.orm = builder.CreateColorOutput(colorDesc);

	/* Emissive */
	colorDesc.format = GBufferLayout::EMISSIVE;
	this->m_output.emissive = builder.CreateColorOutput(colorDesc);

	/* Position */
	colorDesc.format = GBufferLayout::POSITION;
	this->m_output.position = builder.CreateColorOutput(colorDesc);
	
	/* Depth */
	TextureDesc depthDesc = { };
	depthDesc.format = GBufferLayout::DEPTH;
	depthDesc.nWidth = this->m_nWidth;
	depthDesc.nHeight = this->m_nHeight;
	depthDesc.usage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;
	this->m_output.depth = builder.CreateDepthOutput(depthDesc);
}

/**
* Executes the G-Buffer pass
*/
void
GBufferPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
	context->BindPipeline(this->m_pipeline);
	
	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });

	context->BindDescriptorSets(0, { this->m_sceneSet });
	context->BindVertexBuffers({ this->m_vertexBuffer });
	context->BindIndexBuffer({ this->m_indexBuffer });
	context->DrawIndexed(this->m_nIndexCount);
}

/**
* Sets Scene data
* 
* @param sceneSet Scene Descriptor set
* @param vertexBuffer Scene vertex buffer
* @param indexBuffer Scene index buffer
* @param nIndexCount Scene index count
*/
void 
GBufferPass::SetSceneData(
	Ref<DescriptorSet> sceneSet,
	Ref<GPUBuffer> vertexBuffer,
	Ref<GPUBuffer> indexBuffer,
	uint32_t nIndexCount
) {
	this->m_sceneSet = sceneSet;
	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_nIndexCount = nIndexCount;
}