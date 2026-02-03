#include "Core/Renderer/Vulkan/VulkanSwapchain.h"

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	Vector<VkSurfaceFormatKHR> formats;
	Vector<VkPresentModeKHR> presentModes;
};

VulkanSwapchain::VulkanSwapchain(Ref<VulkanDevice> device, VkSurfaceKHR surface) 
	: m_device(device), m_surface(surface), m_swapchain(VK_NULL_HANDLE) {}

VulkanSwapchain::~VulkanSwapchain() {
	VkDevice vkDevice = this->m_device->GetVkDevice();

	if (this->m_swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(vkDevice, this->m_swapchain, nullptr);
	}
}

/**
* Creates a swapchain
*
* @param createInfo Swap chain create info
*/
void 
VulkanSwapchain::Create(const SwapchainCreateInfo& createInfo) {
	this->m_nImageCount = createInfo.nImageCount;
	this->m_pWindow = createInfo.pWindow;

	VkPhysicalDevice physicalDevice = this->m_device->GetVkPhysicalDevice();

	SwapchainSupportDetails details = this->QuerySwapchainSupport(physicalDevice);

	VkSurfaceFormatKHR format = this->ChooseSurfaceFormat(details.formats);
	VkPresentModeKHR presentMode = this->ChooseSwapPresentMode(details.presentModes);
	VkExtent2D extent = this->ChooseSwapExtent(details.capabilities);
	this->m_extent = extent;

	VkSwapchainCreateInfoKHR scInfo = { };
	scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	scInfo.imageFormat = format.format;
	scInfo.imageColorSpace = format.colorSpace;
	scInfo.imageExtent = extent;
	scInfo.imageArrayLayers = 1;
	scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	scInfo.minImageCount = this->m_nImageCount;
	scInfo.surface = this->m_surface;
	scInfo.presentMode = presentMode;
	scInfo.oldSwapchain = VK_NULL_HANDLE;
	scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	scInfo.preTransform = details.capabilities.currentTransform;

	QueueFamilyIndices indices = this->m_device->FindQueueFamilies();
	uint32_t queueIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		scInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		scInfo.queueFamilyIndexCount = 2;
		scInfo.pQueueFamilyIndices = queueIndices;
	}
	else {
		scInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		scInfo.queueFamilyIndexCount = 0;
		scInfo.pQueueFamilyIndices = nullptr;
	}

	VK_CHECK(
		vkCreateSwapchainKHR(
			this->m_device->GetVkDevice(), 
			&scInfo,
			nullptr, 
			&this->m_swapchain
		), 
		"Failed creating swap chain");

	uint32_t nImageCount;
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, nullptr);
	
	Vector<VkImage> images;
	images.resize(nImageCount);
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, images.data());

	for (const VkImage& image : images) {
		Ref<VulkanTexture> vkImage = VulkanTexture::CreateShared(this->m_device);
		vkImage->Create(image);
		this->m_images.push_back(vkImage.As<GPUTexture>());
	}

	this->m_nImageCount = nImageCount;
	Logger::Debug("VulkanSwapchain::Create: Swapchain created. Image count: {}", this->m_nImageCount);

	this->m_imageViews.resize(nImageCount);
	for (uint32_t i = 0; i < nImageCount; i++) {
		VkImageViewCreateInfo viewInfo = { };
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format.format;
		viewInfo.image = images[i];

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK(
			vkCreateImageView(
				this->m_device->GetVkDevice(), 
				&viewInfo, 
				nullptr, 
				&imageView
			), 
			"Failed creating swap chain image view");

		Ref<VulkanImageView> vkView = VulkanImageView::CreateShared(this->m_device->GetVkDevice());
		vkView->Create(imageView);

		this->m_imageViews[i] = vkView.As<ImageView>();
	}

	this->CreateDepthResources();
}

/**
* Acquires the next available image for rendering
*
* @param nTimeout Timeout in nanoseconds (UINT64_MAX = infinite)
* @param pSignalSemaphore Semaphore for when the image is available (optional)
* @param pSignalFence Fence for when the image is available (optional)
*
* @returns Acquired image index or UINT32_MAX if failed
*/
uint32_t 
VulkanSwapchain::AcquireNextImage(
	uint64_t nTimeout,
	void* pSignalSemaphore,
	void* pSignalFence
) {
	uint32_t nImageIndex = 0;

	VkDevice vkDevice = this->m_device->GetVkDevice();
	vkAcquireNextImageKHR(
		vkDevice,
		this->m_swapchain,
		nTimeout,
		static_cast<VkSemaphore>(pSignalSemaphore), static_cast<VkFence>(pSignalFence),
		&nImageIndex
	);

	return nImageIndex;
}

/**
* Presents the actual image in screen
*
* @param nImageIndex Image index
* @param pWaitSemaphores Semaphore vector to wait before presenting (optional)
*
* @returns True if presentation was successful
*/
bool 
VulkanSwapchain::Present(
	uint32_t nImageIndex,
	const Vector<void*>& pWaitSemaphores
) {
	VkQueue queue = this->m_device->GetPresentQueue();

	VkPresentInfoKHR presentInfo = { };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pImageIndices = &nImageIndex;
	presentInfo.pWaitSemaphores = reinterpret_cast<const VkSemaphore*>(pWaitSemaphores.data());
	presentInfo.waitSemaphoreCount = pWaitSemaphores.size();
	presentInfo.pSwapchains = &this->m_swapchain;
	presentInfo.swapchainCount = 1;

	VkResult result = vkQueuePresentKHR(queue, &presentInfo);
	return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
}

/**
* Reconstructs the swap chain
*
* @param nNewWidth New width
* @param nNewHeight New height
*/
void 
VulkanSwapchain::Rebuild(uint32_t nNewWidth, uint32_t nNewHeight) {
	this->m_device->WaitIdle();

	VkSwapchainKHR oldSwapchain = this->m_swapchain;

	/* Clear old resources */
	this->m_imageViews.clear();
	this->m_images.clear();

	this->m_depthImage->Reset();
	this->m_depthImageView->Reset();

	/* Update createInfo with new dimensions */
	this->m_createInfo.width = nNewWidth;
	this->m_createInfo.height = nNewHeight;
	this->m_createInfo.pOldSwapchain = oldSwapchain;

	/* Query updated capabilities */
	VkPhysicalDevice physicalDevice = this->m_device->GetVkPhysicalDevice();
	SwapchainSupportDetails details = this->QuerySwapchainSupport(physicalDevice);

	VkSurfaceFormatKHR format = this->ChooseSurfaceFormat(details.formats);
	VkPresentModeKHR presentMode = this->ChooseSwapPresentMode(details.presentModes);
	VkExtent2D extent = this->ChooseSwapExtent(details.capabilities);

	/* Determine image count */
	if (details.capabilities.maxImageCount > 0 && this->m_nImageCount > details.capabilities.maxImageCount) {
		this->m_nImageCount = details.capabilities.maxImageCount;
	} 

	if (this->m_nImageCount < details.capabilities.minImageCount) {
		this->m_nImageCount = details.capabilities.minImageCount;
	}

	/* Create new swap chain */
	VkSwapchainCreateInfoKHR scInfo = { };
	scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	scInfo.imageFormat = format.format;
	scInfo.imageColorSpace = format.colorSpace;
	scInfo.imageExtent = extent;
	scInfo.imageArrayLayers = 1;
	scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	scInfo.minImageCount = this->m_nImageCount;
	scInfo.surface = this->m_surface;
	scInfo.presentMode = presentMode;
	scInfo.oldSwapchain = oldSwapchain;
	scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	scInfo.preTransform = details.capabilities.currentTransform;

	QueueFamilyIndices indices = this->m_device->FindQueueFamilies();
	uint32_t queueIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		scInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		scInfo.queueFamilyIndexCount = 2;
		scInfo.pQueueFamilyIndices = queueIndices;
	}
	else {
		scInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		scInfo.queueFamilyIndexCount = 0;
		scInfo.pQueueFamilyIndices = nullptr;
	}

	VK_CHECK(
		vkCreateSwapchainKHR(
			this->m_device->GetVkDevice(),
			&scInfo,
			nullptr,
			&this->m_swapchain
		),
		"Failed recreating swap chain");

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(this->m_device->GetVkDevice(), oldSwapchain, nullptr);
	}

	uint32_t nImageCount;
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, nullptr);

	Vector<VkImage> images;
	images.resize(nImageCount);
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, images.data());

	for (const VkImage& image : images) {
		Ref<VulkanTexture> vkImage = VulkanTexture::CreateShared(this->m_device);
		vkImage->Create(image);
		this->m_images.push_back(vkImage.As<GPUTexture>());
	}

	this->m_nImageCount = nImageCount;
	Logger::Debug("VulkanSwapchain::Create: Swapchain recreated. Image count: {}", this->m_nImageCount);

	this->m_imageViews.resize(nImageCount);
	for (uint32_t i = 0; i < nImageCount; i++) {
		VkImageViewCreateInfo viewInfo = { };
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format.format;
		viewInfo.image = images[i];

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VK_CHECK(
			vkCreateImageView(
				this->m_device->GetVkDevice(),
				&viewInfo,
				nullptr,
				&imageView
			),
			"Failed creating swap chain image view");

		Ref<VulkanImageView> vkView = VulkanImageView::CreateShared(this->m_device->GetVkDevice());
		vkView->Create(imageView);

		this->m_imageViews[i] = vkView.As<ImageView>();
	}
}

/**
* Queries swapchain support
* 
* @param physicalDevice Vulkan physical device
* 
* @returns Swap chain support details
*/
SwapchainSupportDetails 
VulkanSwapchain::QuerySwapchainSupport(VkPhysicalDevice physicalDevice) {
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, this->m_surface, &details.capabilities);
	
	uint32_t nFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->m_surface, &nFormatCount, nullptr);

	if (nFormatCount != 0) {
		details.formats.resize(nFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, this->m_surface, &nFormatCount, details.formats.data());
	}

	uint32_t nPresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, this->m_surface, &nPresentModeCount, nullptr);

	if (nPresentModeCount != 0) {
		details.presentModes.resize(nPresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physicalDevice, 
			this->m_surface, 
			&nPresentModeCount, 
			details.presentModes.data()
		);
	}

	return details;
}

/**
* Chooses the best surface format
* 
* @param availableFormats Available formats
* 
* @returns The best surface format
*/
VkSurfaceFormatKHR
VulkanSwapchain::ChooseSurfaceFormat(const Vector<VkSurfaceFormatKHR>& availableFormats) const {
	for (const VkSurfaceFormatKHR& format : availableFormats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return availableFormats[0];
}

/**
* Choose the best swap present mode
* 
* @param presentModes Present modes
* 
* @returns The best swap present mode
*/
VkPresentModeKHR 
VulkanSwapchain::ChooseSwapPresentMode(const Vector<VkPresentModeKHR>& presentModes) {
	for (const VkPresentModeKHR& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

/**
* Finds the depth format
* 
* @returns Vulkan depth format
*/
VkFormat 
VulkanSwapchain::FindDepthFormat() {
	Vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	return this->m_device->FindSupportedFormat(
		candidates,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

/**
* Choose swap extent
* If capabilities extent is not valid, create one
* 
* @param capabilities Surface capabilities
* 
* @returns Swap extent
*/
VkExtent2D 
VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		return capabilities.currentExtent;
	}
	else {
		int nWidth, nHeight;
		glfwGetFramebufferSize(this->m_pWindow, &nWidth, &nHeight);

		VkExtent2D extent = {
			static_cast<uint32_t>(nWidth),
			static_cast<uint32_t>(nHeight)
		};

		extent.width = std::clamp(
			extent.width, 
			capabilities.minImageExtent.width, 
			capabilities.maxImageExtent.width
		);

		extent.height = std::clamp(
			extent.height, 
			capabilities.minImageExtent.height, 
			capabilities.maxImageExtent.height
		);

		return extent;
	}
}

/**
* Creates depth resources
*/
void 
VulkanSwapchain::CreateDepthResources() {
	VkFormat depthFormat = this->FindDepthFormat();

	Extent3D extent = { };
	extent.width = this->m_extent.width;
	extent.height = this->m_extent.height;
	extent.depth = 1;

	TextureCreateInfo textureInfo = { };
	textureInfo.usage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;
	textureInfo.extent = extent;
	textureInfo.tiling = ETextureTiling::OPTIMAL;
	textureInfo.sharingMode = ESharingMode::EXCLUSIVE;
	textureInfo.format = VulkanHelpers::RevertFormat(depthFormat);
	textureInfo.samples = ESampleCount::SAMPLE_1;
	textureInfo.imageType = ETextureDimensions::TYPE_2D;
	textureInfo.initialLayout = ETextureLayout::UNDEFINED;

	Ref<VulkanTexture> depthImage = VulkanTexture::CreateShared(this->m_device);
	depthImage->Create(textureInfo);

	ImageViewCreateInfo viewInfo = { };
	viewInfo.image = depthImage.As<GPUTexture>();
	viewInfo.viewType = EImageViewType::TYPE_2D;
	viewInfo.format = VulkanHelpers::RevertFormat(depthFormat);

	viewInfo.subresourceRange.aspectMask = EImageAspect::DEPTH;
	viewInfo.subresourceRange.nBaseArrayLayer = 0;
	viewInfo.subresourceRange.nLayerCount = 1;
	viewInfo.subresourceRange.nBaseMipLevel = 0;
	viewInfo.subresourceRange.nLevelCount = 1;

	viewInfo.components.r = ComponentMapping::ESwizzle::IDENTITY;
	viewInfo.components.g = ComponentMapping::ESwizzle::IDENTITY;
	viewInfo.components.b = ComponentMapping::ESwizzle::IDENTITY;
	viewInfo.components.a = ComponentMapping::ESwizzle::IDENTITY;

	Ref<VulkanImageView> depthImageView = VulkanImageView::CreateShared(this->m_device->GetVkDevice());
	depthImageView->Create(viewInfo);

	this->m_depthImage = depthImage.As<GPUTexture>();
	this->m_depthImageView = depthImageView.As<ImageView>();
	this->m_depthFormat = VulkanHelpers::RevertFormat(depthFormat);

	this->m_device->TransitionLayout(
		this->m_depthImage,
		this->m_depthFormat, 
		EImageLayout::UNDEFINED,
		EImageLayout::DEPTH_STENCIL_ATTACHMENT
	);
}