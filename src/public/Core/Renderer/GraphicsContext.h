#pragma once
#include <iostream>
#include "Core/Containers.h"

#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPURingBuffer.h"
#include "Core/Renderer/PipelineLayout.h"
#include "Core/Renderer/Viewport.h"
#include "Core/Renderer/Rect2D.h"
#include "Core/Renderer/RenderPass.h"

class GraphicsContext {
public:
	static constexpr const char* CLASS_NAME = "GraphicsContext";

	using Ptr = Ref<GraphicsContext>;

	virtual ~GraphicsContext() = default;

	/**
	* Binds a pipeline
	* 
	* @param pipeline Binded pipeline
	*/
	virtual void BindPipeline(Ref<Pipeline> pipeline) = 0;

	/**
	* Binds descriptor sets
	* 
	* @param nFirstSet First set
	* @param sets DescriptorSet vector
	* @param dynamicOffsets Dynamic offsets (optional)
	*/
	virtual void BindDescriptorSets(
		uint32_t nFirstSet, 
		const Vector<Ref<DescriptorSet>>& sets, const Vector<uint32_t>& dynamicOffsets = {}) = 0;

	/**
	* Binds vertex buffers
	* 
	* @param buffers Vertex buffer vector
	* @param offsets Offsets (default = {})
	*/
	virtual void BindVertexBuffers(const Vector<Ref<GPUBuffer>>& buffers, const Vector<size_t>& offsets = {}) = 0;
	
	/**
	* Binds a index buffer
	* 
	* @param buffer Index buffer
	* @param indexType Index type (defaul = UINT16)
	*/
	virtual void BindIndexBuffer(Ref<GPUBuffer> buffer, EIndexType indexType = EIndexType::UINT16) = 0;

	/**
	* Performs a draw call
	* 
	* @param nVertexCount Number of vertices
	* @param nInstanceCount Number of instances (default = 1)
	* @param nFirstVertex First vertex (default = 0)
	* @param nFirstInstance First instance (default = 0)
	*/
	virtual void Draw(
		uint32_t nVertexCount, 
		uint32_t nInstanceCount = 1,
		uint32_t nFirstVertex = 0, 
		uint32_t nFirstInstance = 0) = 0;

	/**
	* Performs a indexed draw call
	* 
	* @param nIndexCount Number of indices
	* @param nInstanceCount Number of instances (default = 1)
	* @param nFirstIndex First index (default = 0)
	* @param nVertexOffset Vertex offset (default = 0)
	* @param nFirstInstance First instance (default = 0)
	*/
	virtual void DrawIndexed(
		uint32_t nIndexCount,
		uint32_t nInstanceCount = 1, 
		uint32_t nFirstIndex = 0, 
		uint32_t nVertexOffset = 0, 
		uint32_t nFirstInstance = 0) = 0;

	/**
	* Performs an indirect indexed draw call
	* 
	* @param buffer Buffer containing draw parameters
	* @param nOffset Byte offset into buffer where parameters begin
	* @param countBuffer Buffer containing draw count
	* @param nCountBufferOffset Byte offset into countBuffer where the draw count begins
	* @param nMaxDrawCount Maximum number of draws that will be executed
	* @param nStride Byte stride between successive sets of draw parameters.
	*/
	virtual void DrawIndexedIndirect(
		Ref<GPUBuffer> buffer,
		uint32_t nOffset,
		Ref<GPUBuffer> countBuffer,
		uint32_t nCountBufferOffset,
		uint32_t nMaxDrawCount,
		uint32_t nStride) = 0;

	/**
	* Push constants to pipeline layout
	* 
	* @param layout Pipeline layout
	* @param stages Shader stage
	* @param nOffset Push constant offset
	* @param nSize Size of push constant data
	* @param pcData Constant pointer to push constant data
	*/
	virtual void PushConstants(
		Ref<PipelineLayout> layout, 
		EShaderStage stages, 
		uint32_t nOffset, 
		uint32_t nSize, 
		const void* pcData) = 0;

	/**
	* Set viewport
	* 
	* @param viewport Viewport struct
	*/
	virtual void SetViewport(const Viewport& viewport) = 0;
	
	/**
	* Set scissor
	* 
	* @param scissor Scissor rect
	*/
	virtual void SetScissor(const Rect2D& scissor) = 0;

	/**
	* Begins a render pass
	* 
	* @param beginInfo Render pass begin info
	*/
	virtual void BeginRenderPass(const RenderPassBeginInfo& beginInfo) = 0;

	/**
	* Ends the current render pass
	*/
	virtual void EndRenderPass() = 0;

	/**
	* Advances to the next subpass
	*/
	virtual void NextSubpass() = 0;

	/**
	* Fills a buffer
	* 
	* @param buffer Buffer to fill
	* @param nOffset Offset
	* @param nSize Size of the fill
	* @param nData Fill data 
	*/
	virtual void FillBuffer(Ref<GPUBuffer> buffer, uint32_t nOffset, uint32_t nSize, uint32_t nData) = 0;

	/**
	* Dispatches compute work items
	* 
	* @param x X dimension
	* @param y Y dimension
	* @param z Z dimension
	*/
	virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

	/**
	* Buffer memory barrier
	* 
	* @param buffer Buffer
	* @param srcAccess Source access mask
	* @param dstAccess Destination access mask
	*/
	virtual void BufferMemoryBarrier(Ref<GPUBuffer> buffer, EAccess srcAccess, EAccess dstAccess) = 0;
};