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

	VkSwapchainCreateInfoKHR scInfo = { };
	scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	scInfo.imageFormat = format.format;
	scInfo.imageColorSpace = format.colorSpace;
	scInfo.imageExtent = extent;
	scInfo.imageArrayLayers = 1;
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

	uint32_t nImageCount;
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, nullptr);
	
	Vector<VkImage> images;
	images.resize(nImageCount);
	vkGetSwapchainImagesKHR(this->m_device->GetVkDevice(), this->m_swapchain, &nImageCount, images.data());

	for (const VkImage& image : images) {
		Ref<VulkanTexture> vkImage = VulkanTexture::CreateShared(this->m_device->GetVkDevice());
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