#pragma once
#include <cstdint>
#include <memory>

#include "Core/Containers.h"
#include "Core/Renderer/CommandPool.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Swapchain.h"

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

class Device : public std::enable_shared_from_this<Device> {
public:
	static constexpr const char* CLASS_NAME = "Device";

	using Ptr = Ref<Device>;

	virtual ~Device() = default;

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
	virtual Ref<CommandPool> CreateCommandPool(
		const CommandPoolCreateInfo& createInfo, 
		EQueueType queueType = EQueueType::GRAPHICS
	) = 0;

	/**
	* Creates a graphics context
	* 
	* @param commandPool The command pool where the command buffer will live
	* 
	* @returns A graphics context
	*/
	virtual Ref<GraphicsContext> CreateContext(Ref<CommandPool>& commandPool) = 0;

	/**
	* Creates a pipeline layout
	* 
	* @param createInfo Pipeline layout create info
	* 
	* @returns Created pipeline layout
	*/
	virtual Ref<PipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) = 0;

	/**
	* Creates a graphics pipeline
	* 
	* @param createInfo A graphics pipeline create info
	* 
	* @returns Created graphics pipeline
	*/
	virtual Ref<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) = 0;

	/**
	* Creates a compute pipeline
	*
	* @param createInfo A compute pipeline create info
	*
	* @returns Created compute pipeline
	*/
	virtual Ref<Pipeline> CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) = 0;

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

	/**
	* Checks if the format has stencil component
	* 
	* @param format Format
	* 
	* @returns True if has stencil component
	*/
	virtual bool HasStencilComponent(GPUFormat format) = 0;

	/**
	* Transitions a image layout to a new layout
	* 
	* @param image The transitioned image
	* @param format Image format
	* @param oldLayout Old image layout
	* @param newLayout New image layout
	* 
	* @param nLayerCount Layer count (optional)
	* @param nBaseMipLevel Base mip level (optional)
	*/
	virtual void TransitionLayout(
		Ref<GPUTexture> image,
		GPUFormat format,
		EImageLayout oldLayout,
		EImageLayout newLayout,
		uint32_t nLayerCount = 1,
		uint32_t nBaseMipLevel = 0
	) = 0;

	/**
	* Creates a swapchain
	* 
	* @param createInfo Swapchain create info
	* 
	* @returns Created swapchain
	*/
	virtual Ref<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) = 0;

	/**
	* Creates a render pass
	* 
	* @param createInfo Render pass create info
	* 
	* @returns Created render pass
	*/
	virtual Ref<RenderPass> CreateRenderPass(const RenderPassCreateInfo& createInfo) = 0;
};