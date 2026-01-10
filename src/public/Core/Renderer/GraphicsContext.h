#pragma once
#include <iostream>
#include "Core/Containers.h"

#include "Core/Renderer/Pipeline.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPURingBuffer.h"

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


};