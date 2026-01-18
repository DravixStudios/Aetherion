#pragma once
#include <cstdint>

#include "Core/Containers.h"
#include "Core/Renderer/CommandPool.h"
#include "Core/Renderer/GraphicsContext.h"

enum class EQueueType {
	GRAPHICS,
	COMPUTE,
	TRANSFER,
	PRESENT
};

struct DeviceCreateInfo {
	Vector<const char*> requiredExtensions;
	bool bEnableGeometryShader = false;
	bool bEnableTessellationShader = false;
	bool bEnableSamplerAnisotroply = false;
	bool bEnableMultiDrawIndirect = false;
	bool bEnableDepthClamp = false;
	Vector<const char*> validationLayers; // DEBUG ONLY
};

class Device {
public:
	static constexpr const char* CLASS_NAME = "Device";

	using Ptr = Ref<Device>;

	virtual ~Device();

	/**
	* Creates a logical device
	* 
	* @param createInfo Device create info
	*/
	virtual void Create(const DeviceCreateInfo& createInfo) = 0;

	/**
	* Creates a command pool
	* 
	* @param createInfo Command pool create info
	*
	* @returns Created command pool
	*/
	virtual Ref<CommandPool> CreateCommandPool(const CommandPoolCreateInfo& createInfo);

	/**
	* Creates a graphics context
	* 
	* @param commandPool The command pool where the command buffer will live
	* 
	* @returns A graphics context
	*/
	virtual Ref<GraphicsContext> CreateContext(Ref<CommandPool>& commandPool);

	/**
	* Creates a pipeline layout
	* 
	* @param createInfo Pipeline layout create info
	* 
	* @returns Created pipeline layout
	*/
	virtual Ref<PipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo);

	/**
	* Creates a graphics pipeline
	* 
	* @param createInfo A graphics pipeline create info
	* 
	* @returns Created graphics pipeline
	*/
	virtual Ref<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo);

	/**
	* Creates a compute pipeline
	*
	* @param createInfo A compute pipeline create info
	*
	* @returns Created compute pipeline
	*/
	virtual Ref<Pipeline> CreateComputePipeline(const ComputePipelineCreateInfo& createInfo);

	/**
	* Begins a single time command buffer
	*/
	virtual Ref<CommandBuffer> BeginSingleTimeCommandBuffer() = 0;

	/**
	* Ends a single time command buffer
	* 
	* @param commandBuffer The command buffer to end
	*/
	virtual void EndSingleTimeCommandBuffer(Ref<CommandBuffer> commandBuffer) = 0;


	/**
	* Waits till device finishes all the operations
	*/
	virtual void WaitIdle() = 0;

	/**
	* Gets device limits
	*/
	virtual void GetLimits(
		uint32_t& nMaxUniformBufferRange,
		uint32_t& nMaxStorageBufferRange,
		uint32_t& nMaxPushContantsSize,
		uint32_t& nMaxBoundDescriptorSets
	) const = 0;

	/**
	* Gets the Device name
	* 
	* @returns Device name
	*/
	virtual const char* GetDeviceName() const = 0;
};