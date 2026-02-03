#pragma once
#include "Utils.h"
#include "Core/Containers.h"
#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/PipelineLayout.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/GraphicsContext.h"

class RenderGraphBuilder;
class RenderGraphContext;

class BasePass {
public:
	virtual ~BasePass() = default;

	virtual void Init(Ref<Device> device) = 0;
	virtual void SetupNode(RenderGraphBuilder& builder) = 0;
	virtual void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx) = 0;

	/**
	* Sets pass dimensions
	* 
	* @param nWidth Width
	* @param nHeight Height
	*/
	virtual void SetDimensions(uint32_t nWidth, uint32_t nHeight) {
		this->m_nWidth = nWidth;
		this->m_nHeight = nHeight;
	}

protected:
	uint32_t m_nWidth = 0;
	uint32_t m_nHeight = 0;

	Ref<Device> m_device;
	Ref<Pipeline> m_pipeline;
	Ref<PipelineLayout> m_pipelineLayout;
};