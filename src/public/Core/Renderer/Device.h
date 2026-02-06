#pragma once
#include <cstdint>
#include <memory>

#include "Core/Containers.h"
#include "Core/Renderer/CommandPool.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/Swapchain.h"
#include "Core/Renderer/Framebuffer.h"
#include "Core/Renderer/Semaphore.h"
#include "Core/Renderer/Fence.h"
#include "Core/Renderer/Pipeline.h"

enum class EQueueType {
	GRAPHICS,
	COMPUTE,
	TRANSFER,
	PRESENT
};

enum class EPipelineStage : uint32_t {
	TOP_OF_PIPE = 1,
	DRAW_INDIRECT = 1 << 1,
	VERTEX_INPUT = 1 << 2,
	VERTEX_SHADER = 1 << 3,
	TESSELLATION_CONTROL = 1 << 3,
	TESSELLATION_EVAL = 1 << 4,
	GEOMETRY = 1 << 5,
	FRAGMENT = 1 << 6,
	EARLY_FRAGMENT_TESTS = 1 << 7,
	LATE_FRAGMENT_TESTS = 1 << 8,
	COLOR_ATTACHMENT_OUTPUT = 1 << 9,
	COMPUTE_SHADER = 1 << 10,
	TRANSFER = 1 << 11,
	BOTTOM_OF_PIPE = 1 << 12,
	HOST = 1 << 13,

	ALL_GRAPHICS = DRAW_INDIRECT | VERTEX_INPUT | VERTEX_SHADER | 
				   TESSELLATION_CONTROL | TESSELLATION_EVAL | GEOMETRY | 
				   FRAGMENT | EARLY_FRAGMENT_TESTS | LATE_FRAGMENT_TESTS | 
				   COLOR_ATTACHMENT_OUTPUT,

	ALL_COMMANDS = ALL_GRAPHICS | COMPUTE_SHADER |TRANSFER | TOP_OF_PIPE | BOTTOM_OF_PIPE | HOST
};

inline EPipelineStage
operator|(EPipelineStage a, EPipelineStage b) {
	return static_cast<EPipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EPipelineStage
operator&(EPipelineStage a, EPipelineStage b) {
	return static_cast<EPipelineStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct SubmitInfo {
	Vector<Ref<Semaphore>> waitSemaphores;
	Vector<EPipelineStage> waitStages;
	Vector<Ref<CommandBuffer>> commandBuffers;
	Vector<Ref<Semaphore>> signalSemaphores;
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
	* Waits for a fence
	* 
	* @param fence Fence
	*/
	virtual void WaitForFence(Ref<Fence> fence) = 0;

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

	/**
	* Creates a buffer
	* 
	* @param createInfo Buffer createInfo
	* 
	* @returns Created buffer
	*/
	virtual Ref<GPUBuffer> CreateBuffer(const BufferCreateInfo& createInfo) = 0;

	/**
	* Creates a texture
	* 
	* @param createInfo Texture create info
	* 
	* @returns Created texture
	*/
	virtual Ref<GPUTexture> CreateTexture(const TextureCreateInfo& createInfo) = 0;
	
	/**
	* Creates a image view
	* 
	* @param createInfo Image view create info
	* 
	* @returns Created image view
	*/
	virtual Ref<ImageView> CreateImageView(const ImageViewCreateInfo& createInfo) = 0;

	/**
	* Creates a framebuffer
	* 
	* @param createInfo Framebuffer create info
	* 
	* @returns Created framebuffer
	*/
	virtual Ref<Framebuffer> CreateFramebuffer(const FramebufferCreateInfo& createInfo) = 0;

	/**
	* Creates a sampler
	* 
	* @param createInfo Sampler create info
	* 
	* @returns Created sampler
	*/
	virtual Ref<Sampler> CreateSampler(const SamplerCreateInfo& createInfo) = 0;

	/**
	* Creates a descriptor pool
	* 
	* @param createInfo Descriptor pool create info
	* 
	* @returns Created descriptor pool
	*/
	virtual Ref<DescriptorPool> CreateDescriptorPool(const DescriptorPoolCreateInfo& createInfo) = 0;

	/**
	* Creates a descriptor set layout
	* 
	* @param createInfo Descriptor set layout create info
	* 
	* @returns Created descriptor set layout
	*/
	virtual Ref<DescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) = 0;

	/**
	* Creates a descriptor set
	* 
	* @param pool Descriptor pool to allocate from
	* @param layout Descriptor set layout
	* 
	* @returns Created descriptor set
	*/
	virtual Ref<DescriptorSet> CreateDescriptorSet(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) = 0;

	/**
	* Creates a semaphore
	* 
	* @returns Created semaphore
	*/
	virtual Ref<Semaphore> CreateSemaphore() = 0;

	/**
	* Creates a fence
	* 
	* @param createInfo Fence create info
	* 
	* @returns Created fence
	*/
	virtual Ref<Fence> CreateFence(const FenceCreateInfo& createInfo) = 0;

	/**
	* Submits a sequence of semaphores or 
	* command buffers to a queue
	* 
	* @param submitInfo Submit info
	* @param fence Fence
	* 
	*/
	virtual void Submit(const SubmitInfo& submitInfo, Ref<Fence> fence) = 0;
};