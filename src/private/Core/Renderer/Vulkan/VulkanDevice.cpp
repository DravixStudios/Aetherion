#include "Core/Renderer/Vulkan/VulkanDevice.h"

VulkanDevice::VulkanDevice(
	VkPhysicalDevice physicalDevice, 
	VkInstance instance, 
	VkSurfaceKHR surface
) : m_physicalDevice(physicalDevice), 
	m_instance(instance),
	m_surface(surface), 
	m_device(VK_NULL_HANDLE), 
	m_devProperties(0),
	m_graphicsQueue(VK_NULL_HANDLE), m_presentQueue(VK_NULL_HANDLE) { }

VulkanDevice::~VulkanDevice() {
	if (this->m_device != VK_NULL_HANDLE) {
		vkDestroyDevice(this->m_device, nullptr);
	}
}

/**
* Creates a Vulkan logical device
* 
* @param createInfo Device create info
*/
void 
VulkanDevice::Create(const DeviceCreateInfo& createInfo) {
	this->CachePhysicalDeviceProperties();
	this->CacheQueueFamilyProperties();

	QueueFamilyIndices indices = this->FindQueueFamilies();

	if (!indices.IsComplete()) {
		Logger::Error("VulkanDevice::Create: Queue family indices not complete");
		throw std::runtime_error("VulkanDevice::Create: Queue family indices not complete");
	}

	float queuePriority = 1.f;

	/* Discard repeated queue families as std::set doesn't allow repeated values */
	std::set<uint32_t> queueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	/* If in same family, will only add one queue create info */
	for (uint32_t nQueueFamily : queueFamilies) {
		VkDeviceQueueCreateInfo queueInfo = { };
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueCount = 1;
		queueInfo.queueFamilyIndex = nQueueFamily;
		queueInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueInfo);
	}

	/* Physical device features */
	VkPhysicalDeviceFeatures deviceFeatures = { };
	deviceFeatures.samplerAnisotropy = createInfo.bEnableSamplerAnisotroply ? VK_TRUE : VK_FALSE;
	deviceFeatures.multiDrawIndirect = createInfo.bEnableMultiDrawIndirect ? VK_TRUE : VK_FALSE;
	deviceFeatures.geometryShader = createInfo.bEnableGeometryShader ? VK_TRUE : VK_FALSE;
	deviceFeatures.tessellationShader = createInfo.bEnableTessellationShader ? VK_TRUE : VK_FALSE;
	deviceFeatures.depthClamp = createInfo.bEnableDepthClamp ? VK_TRUE : VK_FALSE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	/* Vulkan 1.3 Features */
	VkPhysicalDeviceVulkan13Features vulkan13Feats = { };
	vulkan13Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13Feats.dynamicRendering = VK_TRUE;
	vulkan13Feats.pNext = nullptr;

	/* Vulkan 1.2 Features */
	VkPhysicalDeviceVulkan12Features vulkan12Feats = { };
	vulkan12Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12Feats.pNext = &vulkan13Feats;
	vulkan12Feats.descriptorIndexing = VK_TRUE;
	vulkan12Feats.drawIndirectCount = VK_TRUE;
	vulkan12Feats.descriptorBindingPartiallyBound = VK_TRUE;
	vulkan12Feats.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	vulkan12Feats.descriptorBindingVariableDescriptorCount = VK_TRUE;
	vulkan12Feats.runtimeDescriptorArray = VK_TRUE;
	vulkan12Feats.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12Feats.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vulkan12Feats.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	vulkan12Feats.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	vulkan12Feats.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;

	/* Vulkan 1.1 Features */
	VkPhysicalDeviceVulkan11Features vulkan11Feats = { };
	vulkan11Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan11Feats.pNext = &vulkan12Feats;

	VkPhysicalDeviceFeatures2 deviceFeatures2 = { };
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.features = deviceFeatures;
	deviceFeatures2.pNext = &vulkan11Feats;

	/* Logical device create info */
	VkDeviceCreateInfo deviceInfo = { };
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &deviceFeatures2;
	deviceInfo.ppEnabledExtensionNames = createInfo.requiredExtensions.data();
	deviceInfo.enabledExtensionCount = createInfo.requiredExtensions.size();
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.queueCreateInfoCount = queueCreateInfos.size();
	
	if (!createInfo.validationLayers.empty()) {
		deviceInfo.enabledLayerCount = createInfo.validationLayers.size();
		deviceInfo.ppEnabledLayerNames = createInfo.validationLayers.data();
	}
	else {
		deviceInfo.enabledLayerCount = 0;
		deviceInfo.ppEnabledLayerNames = nullptr;
	}

	VK_CHECK(vkCreateDevice(
		this->m_physicalDevice,
		&deviceInfo, 
		nullptr,
		&this->m_device
	 ), "Failed creating logical device");

	/* Store our graphics and present queues on it's respective class members */
	vkGetDeviceQueue(this->m_device, indices.graphicsFamily.value(), 0, &this->m_graphicsQueue);
	vkGetDeviceQueue(this->m_device, indices.presentFamily.value(), 0, &this->m_presentQueue);

	/* Create transfer command pool */
	CommandPoolCreateInfo poolInfo = { };
	poolInfo.nQueueFamilyIndex = indices.graphicsFamily.value();
	poolInfo.flags = ECommandPoolFlags::TRANSIENT;

	this->m_transferPool = this->CreateCommandPool(poolInfo);
}

void 
VulkanDevice::WaitIdle() {

}

/**
* Creates a Vulkan command pool
* 
* Note: In the createInfo parameter, let nQueueFamilyIndex member
* as 0, Device will resolve it from queueType
* 
* @param createInfo Command pool create info
*
* @returns Created command pool
*/
Ref<CommandPool> 
VulkanDevice::CreateCommandPool(const CommandPoolCreateInfo& createInfo, EQueueType queueType) {
	CommandPoolCreateInfo poolInfo = { };
	memcpy(&poolInfo, &createInfo, sizeof(CommandPoolCreateInfo));

	QueueFamilyIndices indices = this->FindQueueFamilies();
	switch (queueType) {
		case EQueueType::GRAPHICS: poolInfo.nQueueFamilyIndex = indices.graphicsFamily.value(); break;
		case EQueueType::PRESENT: poolInfo.nQueueFamilyIndex = indices.presentFamily.value(); break;
		default: poolInfo.nQueueFamilyIndex = indices.graphicsFamily.value(); break;
	}

	Ref<VulkanCommandPool> pool = VulkanCommandPool::CreateShared(this->m_device);
	pool->Create(poolInfo);

	return pool.As<CommandPool>();
}

/**
* Creates a Vulkan graphics context
*
* @param commandPool The command pool where the command buffer will live
*
* @returns A Vulkan graphics context
*/
Ref<GraphicsContext> 
VulkanDevice::CreateContext(Ref<CommandPool>& commandPool) {
	Ref<CommandBuffer> commandBuff = commandPool->AllocateCommandBuffer();

	Ref<VulkanCommandBuffer> vkCommandBuff = commandBuff.As<VulkanCommandBuffer>();

	Ref<VulkanGraphicsContext> context = VulkanGraphicsContext::CreateShared(vkCommandBuff->GetVkCommandBuffer());
	return context.As<GraphicsContext>();
}

/**
* Creates a Vulkan pipeline layout
* 
* @param createInfo Pipeline layout create info
* 
* @returns Created pipeline layout
*/
Ref<PipelineLayout>
VulkanDevice::CreatePipelineLayout(const PipelineLayoutCreateInfo& createInfo){
	Ref<VulkanPipelineLayout> layout = VulkanPipelineLayout::CreateShared(this->m_device);
	layout->Create(createInfo);

	return layout.As<PipelineLayout>();
}

/**
* Creates a Vulkan graphics pipeline
*
* @param createInfo A graphics pipeline create info
*
* @returns Created Vulkan graphics pipeline
*/
Ref<Pipeline> 
VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) {
	Ref<VulkanPipeline> pipeline = VulkanPipeline::CreateShared(this->m_device);
	pipeline->CreateGraphics(createInfo);

	return pipeline.As<Pipeline>();
}

/**
* Creates a Vulkan compute pipeline
*
* @param createInfo A compute pipeline create info
*
* @returns Created Vulkan compute pipeline
*/
Ref<Pipeline> 
VulkanDevice::CreateComputePipeline(const ComputePipelineCreateInfo& createInfo) {
	Ref<VulkanPipeline> pipeline = VulkanPipeline::CreateShared(this->m_device);
	pipeline->CreateCompute(createInfo);
	
	return pipeline.As<Pipeline>();
}

/**
* Begins a Vulkan single time command buffer
*/
Ref<CommandBuffer> 
VulkanDevice::BeginSingleTimeCommandBuffer() {
	Ref<CommandBuffer> commandBuff = this->m_transferPool->AllocateCommandBuffer();
	commandBuff->Begin(true);

	return commandBuff;
}

/**
* Ends a Vulkan single time command buffer
*
* @param commandBuffer The Vulkan command buffer to end
*/
void 
VulkanDevice::EndSingleTimeCommandBuffer(Ref<CommandBuffer> commandBuffer) {
	commandBuffer->End();

	/* TODO: Create a CreateFence method */
	VkFenceCreateInfo fenceInfo = { };
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	/* Create fence */
	VkFence fence;
	VK_CHECK(vkCreateFence(this->m_device, &fenceInfo, nullptr, &fence), "Failed creating fence");

	/* Convert the command buffer */
	Ref<VulkanCommandBuffer> vkCommandBuff = commandBuffer.As<VulkanCommandBuffer>();

	const VkCommandBuffer commandBuffers[] = {
		vkCommandBuff->GetVkCommandBuffer()
	};

	/* Submit command buffer */
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = commandBuffers;
	
	vkQueueSubmit(this->m_graphicsQueue, 1, &submitInfo, fence);

	/* Wait for fence */
	vkWaitForFences(this->m_device, 1, &fence, VK_TRUE, UINT64_MAX);

	/* Cleanup */
	this->m_transferPool->FreeCommandBuffer(commandBuffer);
	vkDestroyFence(this->m_device, fence, nullptr);
}

/**
* Finds a supported format between candidates
* 
* @param candidates Format candidates
* @param tiling Image tiling
* @param flags Format features
* 
* @returns Supported format
* 
* @throws std::runtime_error if no supported format found
*/
VkFormat 
VulkanDevice::FindSupportedFormat(
	const Vector<VkFormat>& candidates, 
	VkImageTiling tiling, 
	VkFormatFeatureFlags flags
) {
	for (const VkFormat& format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(this->m_physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & flags) == flags) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & flags) == flags) {
			return format;
		}
	}

	Logger::Error("VulkanDevice::FindSupportedFormat: Failed finding a optimal format candidate");
	throw std::runtime_error("VulkanDevice::FindSupportedFormat: Failed finding a optimal format candidate");
}

/**
* Gets device limits
*/
void 
VulkanDevice::GetLimits(
	uint32_t& nMaxUniformBufferRange,
	uint32_t& nMaxStorageBufferRange,
	uint32_t& nMaxPushConstantSize,
	uint32_t& nMaxBoundDescriptorSets
) const {
	VkPhysicalDeviceLimits limits = this->m_devProperties.limits;
	
	nMaxUniformBufferRange = limits.maxUniformBufferRange;
	nMaxStorageBufferRange = limits.maxStorageBufferRange;
	nMaxPushConstantSize = limits.maxPushConstantsSize;
	nMaxBoundDescriptorSets = limits.maxBoundDescriptorSets;
}

/**
* Ges the device name
* 
* @returns Device name
*/
const char* 
VulkanDevice::GetDeviceName() const {
	return this->m_devProperties.deviceName;
}

/**
* Checks if the format has stencil component
*
* @param format Format
*
* @returns True if has stencil component
*/
bool 
VulkanDevice::HasStencilComponent(GPUFormat format) {
	return format == GPUFormat::D32_FLOAT_S8_UINT || format == GPUFormat::D24_UNORM_S8_UINT;
}

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
void 
VulkanDevice::TransitionLayout(
	Ref<GPUTexture> image,
	GPUFormat format,
	EImageLayout oldLayout,
	EImageLayout newLayout,
	uint32_t nLayerCount,
	uint32_t nBaseMipLevel
) {
	Ref<VulkanCommandBuffer> commandBuffer = this->BeginSingleTimeCommandBuffer().As<VulkanCommandBuffer>();
	VkImage vkImage = image.As<VulkanTexture>()->GetVkImage();

	/* Create a barrier */
	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VulkanHelpers::ConvertImageLayout(oldLayout);
	barrier.newLayout = VulkanHelpers::ConvertImageLayout(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;

	/* Barrier subresource range definition */
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = nLayerCount;
	barrier.subresourceRange.baseMipLevel = nBaseMipLevel;
	barrier.subresourceRange.levelCount = 1;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	if (oldLayout == EImageLayout::UNDEFINED && newLayout == EImageLayout::TRANSFER_DST) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == EImageLayout::TRANSFER_DST && newLayout == EImageLayout::SHADER_READ_ONLY) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == EImageLayout::UNDEFINED && newLayout == EImageLayout::DEPTH_STENCIL_ATTACHMENT) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		
		if (this->HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == EImageLayout::UNDEFINED && newLayout == EImageLayout::COLOR_ATTACHMENT) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == EImageLayout::COLOR_ATTACHMENT && newLayout == EImageLayout::TRANSFER_SRC) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == EImageLayout::TRANSFER_DST && newLayout == EImageLayout::PRESENT_SRC) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = 0;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == EImageLayout::DEPTH_STENCIL_ATTACHMENT && newLayout == EImageLayout::SHADER_READ_ONLY) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == EImageLayout::COLOR_ATTACHMENT && newLayout == EImageLayout::SHADER_READ_ONLY) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		Logger::Error("VulkanDevice::TransitionLayout: Unsupported layout transition");
		throw std::runtime_error("VulkanDevice::TransitionLayout: Unsupported layout transition");
	}

	vkCmdPipelineBarrier(
		commandBuffer->GetVkCommandBuffer(),
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	this->EndSingleTimeCommandBuffer(commandBuffer.As<CommandBuffer>());
}

/**
* Creates Vulkan a swapchain
*
* @param createInfo Swapchain create info
*
* @returns Created Vulkan swapchain
*/
Ref<Swapchain> 
VulkanDevice::CreateSwapchain(const SwapchainCreateInfo& createInfo) {
	if (!createInfo.pWindow) {
		Logger::Error("VulkanDevice::CreateSwapchain: createInfo.pWindow is null");
		throw std::runtime_error("VulkanDevice::CreateSwapchain: createInfo.pWindow is null");
	}

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VK_CHECK(
		glfwCreateWindowSurface(
			this->m_instance,
			createInfo.pWindow,
			nullptr,
			&surface),
		"Failed to create window surface");

	Ref<VulkanDevice> deviceRef = std::static_pointer_cast<VulkanDevice>(this->shared_from_this());

	Ref<VulkanSwapchain> swapchain = VulkanSwapchain::CreateShared(deviceRef, surface);
	swapchain->Create(createInfo);

	return swapchain.As<Swapchain>();
}

/**
* Creates a Vulkan render pass
*
* @param createInfo Render pass create info
*
* @returns Created Vulkan render pass
*/
Ref<RenderPass> 
VulkanDevice::CreateRenderPass(const RenderPassCreateInfo& createInfo) {
	Ref<VulkanRenderPass> renderPass = VulkanRenderPass::CreateShared(this->m_device);
	renderPass->Create(createInfo);

	return renderPass.As<RenderPass>();
}

/**
* Creates a Vulkan texture
*
* @param createInfo Texture create info
*
* @returns Created Vulkan texture
*/
Ref<GPUTexture> 
VulkanDevice::CreateTexture(const TextureCreateInfo& createInfo) {
	Ref<VulkanDevice> deviceRef = std::static_pointer_cast<VulkanDevice>(this->shared_from_this());

	Ref<VulkanTexture> texture = VulkanTexture::CreateShared(deviceRef);
	texture->Create(createInfo);
	return texture.As<GPUTexture>();
}

/**
* Creates a Vulkan image view
*
* @param createInfo Image view create info
*
* @returns Created Vulkan image view
*/
Ref<ImageView> 
VulkanDevice::CreateImageView(const ImageViewCreateInfo& createInfo) {
	Ref<VulkanImageView> imageView = VulkanImageView::CreateShared(this->m_device);
	imageView->Create(createInfo);
	return imageView.As<ImageView>();
}

/**
* Creates a Vulkan framebuffer
*
* @param createInfo Framebuffer create info
*
* @returns Created Vulkan framebuffer
*/
Ref<Framebuffer> 
VulkanDevice::CreateFramebuffer(const FramebufferCreateInfo& createInfo) {
	Ref<VulkanFramebuffer> framebuffer = VulkanFramebuffer::CreateShared(this->m_device);
	framebuffer->Create(createInfo);

	return framebuffer.As<Framebuffer>();
}

/**
* Creates a Vulkan sampler
*
* @param createInfo Sampler create info
*
* @returns Created Vulkan sampler
*/
Ref<Sampler> 
VulkanDevice::CreateSampler(const SamplerCreateInfo& createInfo) {
	Ref<VulkanDevice> deviceRef = std::static_pointer_cast<VulkanDevice>(this->shared_from_this());
	Ref<VulkanSampler> sampler = VulkanSampler::CreateShared(deviceRef);
	sampler->Create(createInfo);

	return sampler.As<Sampler>();
}

/**
* Creates a Vulkan descriptor pool
*
* @param createInfo Descriptor pool create info
*
* @returns Created Vulkan descriptor pool
*/
Ref<DescriptorPool> 
VulkanDevice::CreateDescriptorPool(const DescriptorPoolCreateInfo& createInfo) {
	Ref<VulkanDescriptorPool> pool = VulkanDescriptorPool::CreateShared(this->m_device);
	pool->Create(createInfo);
	
	return pool.As<DescriptorPool>();
}

/**
* Creates a Vulkan descriptor set layout
*
* @param createInfo Descriptor set layout create info
*
* @returns Created Vulkan descriptor set layout
*/
Ref<DescriptorSetLayout> 
VulkanDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo) {
	Ref<VulkanDescriptorSetLayout> layout = VulkanDescriptorSetLayout::CreateShared(this->m_device);
	layout->Create(createInfo);

	return layout.As<DescriptorSetLayout>();
}

/**
* Creates a Vulkan descriptor set
*
* @param pool Descriptor pool to allocate from
* @param layout Descriptor set layout
*
* @returns Created Vulkan descriptor set
*/
Ref<DescriptorSet> 
VulkanDevice::CreateDescriptorSet(Ref<DescriptorPool> pool, Ref<DescriptorSetLayout> layout) {
	Ref<VulkanDescriptorSet> set = VulkanDescriptorSet::CreateShared(this->m_device);
	set->Allocate(pool, layout);

	return set.As<DescriptorSet>();
}

/**
* Creates a Vulkan semaphore
*
* @returns Created Vulkan semaphore
*/
Ref<Semaphore> 
VulkanDevice::CreateSemaphore() {
	Ref<VulkanDevice> deviceRef = std::static_pointer_cast<VulkanDevice>(this->shared_from_this());
	Ref<VulkanSemaphore> semaphore = VulkanSemaphore::CreateShared(deviceRef);
	semaphore->Create();

	return semaphore.As<Semaphore>();
}

/**
* Finds memory type
* 
* @param typeFilter Type filter
* @param properties Memory property flags
* 
* @returns Memory type
*/
uint32_t 
VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProps = { };
	vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevice, &memProps);

	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	}

	Logger::Error("VulkanDevice::FindMemoryType: Error finding memory type");
	throw std::runtime_error("VulkanDevice::FindMemoryType: Error finding memory type");
	return UINT32_MAX;
}

/**
* Caches queue family properties
*/
void 
VulkanDevice::CacheQueueFamilyProperties() {
	uint32_t nQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(this->m_physicalDevice, &nQueueFamilyCount, nullptr);

	this->m_queueFamilyProperties.resize(nQueueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(
		this->m_physicalDevice, 
		&nQueueFamilyCount, 
		this->m_queueFamilyProperties.data()
	);
}

/**
* Finds queue family indices
* 
* @returns Queue family indices
*/
QueueFamilyIndices 
VulkanDevice::FindQueueFamilies() {
	QueueFamilyIndices indices;

	uint32_t i = 0;
	for (const VkQueueFamilyProperties& queueFamily : this->m_queueFamilyProperties) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(this->m_physicalDevice, i, this->m_surface, &bPresentSupport);

		if (bPresentSupport) {
			indices.presentFamily = i;
		}

		if (indices.IsComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

/**
* Caches Vulkan physical device properties
*/
void
VulkanDevice::CachePhysicalDeviceProperties() {
	VkPhysicalDeviceProperties devProps = { };
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &devProps);

	this->m_devProperties = devProps;
}