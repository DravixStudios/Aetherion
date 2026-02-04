#include "Core/Renderer/Rendering/Passes/LightingPass.h"

/**
* Lighting pass initialization
*/
void 
LightingPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Lighting pass node setup
*/
void
LightingPass::SetupNode(RenderGraphBuilder& builder) {
	builder.ReadTexture(this->m_input.albedo);
	builder.ReadTexture(this->m_input.normal);
	builder.ReadTexture(this->m_input.orm);
	builder.ReadTexture(this->m_input.emissive);
	builder.ReadTexture(this->m_input.position);
	builder.ReadTexture(this->m_input.depth);

	TextureDesc texDesc = { };
	texDesc.format = GPUFormat::RGBA16_FLOAT;
	texDesc.nWidth = this->m_nWidth;
	texDesc.nHeight = this->m_nHeight;
	texDesc.usage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;

	this->m_output.lightingOutput = builder.CreateColorOutput(texDesc);
}

/**
* Executes the lighting pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
*/
void
LightingPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) {
	context->BindPipeline(this->m_pipeline);

	Viewport vp { 0.f, 0.f, static_cast<float>(this->m_nWidth), static_cast<float>(this->m_nHeight), 0.f, 1.f };
	context->SetViewport(vp);
	context->SetScissor({ { 0, 0 }, { this->m_nWidth, this->m_nHeight } });

	context->BindDescriptorSets(0, { this->m_gbufferSet, this->m_lightSet });
	context->BindVertexBuffers({ this->m_vertexBuffer });

	context->BindIndexBuffer(this->m_indexBuffer);
	context->DrawIndexed(this->m_nIndexCount);
}

/**
* Sets lighting pass input
* 
* @param gbOutput G-Buffer output
*/
void
LightingPass::SetInput(const GBufferPass::Output& gbOutput) {
	this->m_input.albedo = gbOutput.albedo;
	this->m_input.normal = gbOutput.normal;
	this->m_input.orm = gbOutput.orm;
	this->m_input.emissive = gbOutput.emissive;
	this->m_input.position = gbOutput.position;
	this->m_input.depth = gbOutput.depth;
}

/**
* Sets the light data
* 
* @param lightSet Light descriptor set
* @param vertexBuffer Vertex buffer (For screenquad)
* @param indexBuffer Index bufer (For screenquad)
* @param nIndexCount Index count (For screenquad)
*/
void 
LightingPass::SetLightData(
	Ref<DescriptorSet> lightSet,
	Ref<GPUBuffer> vertexBuffer,
	Ref<GPUBuffer> indexBuffer,
	uint32_t nIndexCount
) {
	this->m_lightSet = lightSet;
	this->m_vertexBuffer = vertexBuffer;
	this->m_indexBuffer = indexBuffer;
	this->m_nIndexCount = nIndexCount;
}

/**
* Sets the G-Buffer descriptor set
* 
* @param gbSet G-Buffer descriptor set
*/
void
LightingPass::SetGBufferDescriptorSet(Ref<DescriptorSet> gbSet) {
	this->m_gbufferSet = gbSet;
}