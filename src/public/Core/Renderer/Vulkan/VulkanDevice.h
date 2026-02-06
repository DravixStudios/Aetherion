#pragma once
#include <optional>
#include <set>

#include "Utils.h"
#include "Core/Logger.h"

#include "Core/Renderer/Device.h"

#include "Core/Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Renderer/Vulkan/VulkanCommandPool.h"
#include "Core/Renderer/Vulkan/VulkanCommandBuffer.h"
#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"
#include "Core/Renderer/Vulkan/VulkanPipelineLayout.h"
#include "Core/Renderer/Vulkan/VulkanSwapchain.h"
#include "Core/Renderer/Vulkan/VulkanFramebuffer.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorSet.h"
#include "Core/Renderer/Vulkan/VulkanDescriptorSetLayout.h"
#include "Core/Renderer/Vulkan/VulkanSampler.h"
#include "Core/Renderer/Vulkan/VulkanSemaphore.h"
#include "Core/Renderer/Vulkan/VulkanFence.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanDevice : public Device {
public:
	using Ptr = Ref<VulkanDevice>;

	explicit VulkanDevice(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
	~VulkanDevice() override;

	void Create(const DeviceCreateInfo& createInfo) override;
	void WaitIdle() override;

	Ref<CommandPool> CreateCommandPool(
		const CommandPoolCreateInfo& createInfo, 
		EQueueType queueType = EQueueType::GRAPHICS
	) override;

	Ref<GraphicsContext> CreateContext(Ref<CommandPool>& commandPool) override;

	Ref<PipelineLayout> CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo) override;

	Ref<Pipeline> CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) override;
	Ref<Pipeline> CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) override;

	Ref<CommandBuffer> BeginSingleTimeCommandBuffer() override;
	void EndSingleTimeCommandBuffer(Ref<CommandBuffer> commandBuffer) override;

	VkFormat FindSupportedFormat(const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags flags);

	void GetLimits(
		uint32_t& nMaxUniformBufferRange,
		uint32_t& nMaxStorageBufferRange,
		uint32_t& nMaxPushConstantSize,
		uint32_t& nMaxBoundDescriptorSets
	) const override;

	const char* GetDeviceName() const override;

	bool HasStencilComponent(GPUFormat format) override;
	void TransitionLayout(
		Ref<GPUTexture> image,
		GPUFormat format,
		EImageLayout oldLayout, 
		EImageLayout newLayout,
		uint32_t nLayerCount = 1,
		uint32_t nBaseMipLevel = 0
	) override;

	Ref<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) override;

	Ref<RenderPass> CreateRenderPass(const RenderPassCreateInfo& createInfo) override;

	Ref<GPUTexture> CreateTexture(const TextureCreateInfo& createInfo) override;

	Ref<ImageView> CreateImageView(const ImageViewCreateInfo& createInfo) override;

	Ref<Framebuffer> CreateFramebuffer(const FramebufferCreateInfo& createInfo) override;

	Ref<Sampler> CreateSampler(const SamplerCreateInfo& createInfo) override;

	Ref<DescriptorPool> CreateDescriptorPool(const DescriptorPoolCreateInfo& createInfo) override;

	Ref<DescriptorSetLayout> CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) override;

	Ref<DescriptorSet> CreateDescriptorSet(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) override;

	Ref<Semaphore> CreateSemaphore() override;
	Ref<Fence> CreateFence(const FenceCreateInfo& createInfo) override;

	VkDevice GetVkDevice() const { return this->m_device; }
	VkPhysicalDevice GetVkPhysicalDevice() const { return this->m_physicalDevice; }

	VkQueue GetPresentQueue() const { return this->m_presentQueue; }
	VkQueue GetGraphicsQueue() const { return this->m_graphicsQueue; }

	/**
	* Gets physical device properties
	*
	* @returns Physical device prope
	*/
	VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return this->m_devProperties; }

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	
	QueueFamilyIndices FindQueueFamilies();

	static Ptr
	CreateShared(VkPhysicalDevice physicalDevice, VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE) {
		return CreateRef<VulkanDevice>(physicalDevice, instance, surface);
	}
private:
	VkPhysicalDevice m_physicalDevice;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkDevice m_device;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	Ref<CommandPool> m_transferPool;
	
	Vector<VkQueueFamilyProperties> m_queueFamilyProperties;
	void CacheQueueFamilyProperties();

	VkPhysicalDeviceProperties m_devProperties;

	void CachePhysicalDeviceProperties();

};