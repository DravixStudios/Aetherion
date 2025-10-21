#include "Core/Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Scene/SceneManager.h"

/* Validation layers (hard-coded) */
std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

/* Device extensions (hard-coded) */
std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

/* Queue family indices helper struct */
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

/* Swap chain support details helper struct */
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

/* Convert from GPUFormat to VkFormat */
VkFormat ToVkFormat(GPUFormat format) {
	switch (format) {
		case GPUFormat::RGBA8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case GPUFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case GPUFormat::RGBA16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case GPUFormat::RGBA32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case GPUFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case GPUFormat::D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
		case GPUFormat::D32_FLOAT_S8_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case GPUFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
	}
}

/* Constructor */
VulkanRenderer::VulkanRenderer() : Renderer::Renderer() {
	this->m_bEnableValidationLayers = ENABLE_VALIDATION_LAYERS;
	this->m_vkInstance = nullptr;
	this->m_debugMessenger = nullptr;
	this->m_physicalDevice = nullptr;
	this->m_surface = nullptr;
	this->m_device = nullptr;
	this->m_graphicsQueue = nullptr;
	this->m_presentQueue = nullptr;
	this->m_renderPass = nullptr;
	this->m_sc = nullptr;
	this->m_scExtent = VkExtent2D();
	this->m_surfaceFormat = VkFormat::VK_FORMAT_UNDEFINED;
	this->m_fence = nullptr;
	this->m_imageAvailable = nullptr;
	this->m_renderFinished = nullptr;
	this->m_pipeline = nullptr;
	this->m_pipelineLayout = nullptr;
	this->m_scissor = VkRect2D();
	this->m_viewport = VkViewport();
	this->m_commandPool = nullptr;
	this->m_sceneMgr = nullptr;
}

/* Renderer init method */
void VulkanRenderer::Init() {
	Renderer::Init();
	std::vector<Vertex> vertices = {
		{
			{ 0.f, 0.5f, 0.f },
			{ 0.f, 0.f, 0.f },
			{ 0.f, 0.f }
		},
		{
			{ .5f, -.5f, 0.f },
			{ 0.f, 1.f, 0.f },
			{ 0.f, 0.f }
		},
		{
			{ -0.5f, -0.5f, 0.f },
			{ 0.f, 0.f, 0.f },
			{ 0.f, 0.f }
		},
	};

	this->CreateInstance();
	this->SetupDebugMessenger();
	this->CreateSurface();
	this->PickPhysicalDevice();
	this->CreateLogicalDevice();
	this->CreateSwapChain();
	this->CreateImageViews();
	this->CreateRenderPass();
	this->CreateCommandPool();
	this->CreateColorResources();
	this->CreateDepthResources();
	this->CreateFrameBuffers();
	this->CreateCommandBuffer();
	this->CreateGraphicsPipeline();
	this->CreateSyncObjects();

	this->m_sceneMgr = SceneManager::GetInstance();
}

/* Initialize our Vulkan instance */
void VulkanRenderer::CreateInstance() {
	spdlog::debug("Instance creation started");

	/* Check if validation layers enabled and if are supported */
	if (this->m_bEnableValidationLayers && !this->CheckValidationLayersSupport()) {
		spdlog::error("Validation layers required but not available");
		throw std::runtime_error("Validation layers required but not available");
		return;
	}

	/* Vulkan application info */
	VkApplicationInfo appInfo = { };
	appInfo.apiVersion = VK_API_VERSION_1_1;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "No name"; /* Actually not making a game lol. We're only setting pEngineName */
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "Aetherion Engine";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	/* Fetch required extensions and size */
	std::vector<const char*> extensions = this->GetRequiredExtensions();
	size_t nExtensionCount = extensions.size();

	spdlog::debug("Required extension count: {0}", nExtensionCount);

	/* Vulkan instance creation info */
	VkInstanceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = nExtensionCount;
	createInfo.ppEnabledExtensionNames = extensions.data();

	/* If debug layers enabled, use them */
	if (this->m_bEnableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = { };
		this->PopulateDebugMessengerCreateInfo(messengerCreateInfo);

		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&messengerCreateInfo);
		
		spdlog::debug("Debug layers are enabled");
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}

	/* Create the instance */
	if (vkCreateInstance(&createInfo, nullptr, &this->m_vkInstance) != VK_SUCCESS) {
		spdlog::error("Failed creating Vulkan instance");
		throw std::runtime_error("Failed creating Vulkan instance");
		return;
	}
	
	spdlog::debug("Vulkan instance created");
}

/* Set up of our debug messenger */
void VulkanRenderer::SetupDebugMessenger() {
	if (!this->m_bEnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = { };
	this->PopulateDebugMessengerCreateInfo(createInfo);

	if (this->CreateDebugUtilsMessengerEXT(this->m_vkInstance, &createInfo, nullptr, &this->m_debugMessenger) != VK_SUCCESS) {
		spdlog::error("Failed to setup Vulkan debug messenger");
	}
}

/* Window surface creation */
void VulkanRenderer::CreateSurface() {
	if (!this->m_pWindow) {
		spdlog::error("CreateSurface: No window provided");
		throw std::runtime_error("CreateSurface: No window provided");
		return;
	}
	if (glfwCreateWindowSurface(this->m_vkInstance, this->m_pWindow, nullptr, &this->m_surface) != VK_SUCCESS) {
		spdlog::error("CreateSurface: Couldn't create a window");
		throw std::runtime_error("CreateSurface: Couldn't create a window");
	}

	spdlog::debug("GLFW: Window surface created");
}

/* Pick the most suitable physical device */
void VulkanRenderer::PickPhysicalDevice() {
	/* Enumerate the amount of physical devices */
	uint32_t nPhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(this->m_vkInstance, &nPhysicalDeviceCount, nullptr);

	/* Store all the physical devices on a vector */
	std::vector<VkPhysicalDevice> physicalDevices(nPhysicalDeviceCount);
	vkEnumeratePhysicalDevices(this->m_vkInstance, &nPhysicalDeviceCount, physicalDevices.data());

	spdlog::debug("Available physical devices: {0}", nPhysicalDeviceCount);
	
	for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
		if (this->IsDeviceSuitable(physicalDevice)) {
			this->m_physicalDevice = physicalDevice;
			break;
		}
	}

	if (this->m_physicalDevice == nullptr) {
		spdlog::error("PickPhysicalDevice: Failed to find a suitable GPU.");
		throw std::runtime_error("PickPhysicalDevice: Failed to find a suitable GPU.");
		return;
	}

	/* Get physical device properties */
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &deviceProperties);

	/* Check if physical device supports Vulkan 1.1 */
	uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
	uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);

	spdlog::debug("Max supported Vulkan version: {}.{}", apiMajor, apiMinor);

	if (apiMajor < 1 || (apiMajor == 1 && apiMinor < 1)) {
		spdlog::error("PickPhysicalDevice: Selected GPU does not support Vulkan 1.1 minimum (found {}.{})",
			apiMajor, apiMinor);
		throw std::runtime_error("PickPhysicalDevice: Vulkan 1.1 required.");
	}



	spdlog::debug("Selected physical device: {0}", deviceProperties.deviceName);
}

/* Create our logical device */
void VulkanRenderer::CreateLogicalDevice() {
	QueueFamilyIndices indices = this->FindQueueFamilies(this->m_physicalDevice);

	/* If queue family indices are not complete, throw error */
	if (!indices.IsComplete()) {
		spdlog::error("CreateLogicalDevice: Queue family indices not complete");
		throw std::runtime_error("CreateLogicalDevice: Queue family indices not complete");
		return;
	}

	float fQueuePriority = 1.f;

	/* Store all the queue families on a single set. We use a set because it can only store unique values. */
	std::set<uint32_t> queueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	/* If in same family, will only add one queue create info */
	for (uint32_t queueFamily : queueFamilies) {
		/* Device queue create info */
		VkDeviceQueueCreateInfo queueInfo = { };
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &fQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	/* Physical device features */
	VkPhysicalDeviceFeatures deviceFeatures = { };
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	/* Logical device create info */
	VkDeviceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pEnabledFeatures = &deviceFeatures;

	if (this->m_bEnableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	/* Logical device creation */
	if (vkCreateDevice(this->m_physicalDevice, &createInfo, nullptr, &this->m_device) != VK_SUCCESS) {
		spdlog::error("CreateLogicalDevice: Couldn't create logical device");
		throw std::runtime_error("CrateLogicalDevice: Couldn't create logical device");
		return;
	}

	spdlog::debug("Logical device created");

	/* Store our graphics and present queues on it's respective class members */
	vkGetDeviceQueue(this->m_device, indices.graphicsFamily.value(), 0, &this->m_graphicsQueue);
	vkGetDeviceQueue(this->m_device, indices.presentFamily.value(), 0, &this->m_presentQueue);
}

/* Create our swap chain */
void VulkanRenderer::CreateSwapChain() {
	SwapChainSupportDetails details = this->QuerySwapChainSupport(this->m_physicalDevice);

	/* Image count: Minimum supported image count + 1 (if 2, we'll triple buffer it) */
	uint32_t nImageCount = details.capabilities.minImageCount + 1;

	/* Query format, present mode and extent */
	VkSurfaceFormatKHR format = this->ChooseSwapSurfaceFormat(details.formats);
	VkPresentModeKHR presentMode = this->ChooseSwapPresentMode(details.presentModes);
	VkExtent2D extent = this->ChooseSwapExtent(details.capabilities);

	/* If maxImageCount is 0, it means that is infinite image count. Then, if has limit, use the limit */
	if (details.capabilities.maxImageCount > 0 && details.capabilities.maxImageCount < nImageCount) {
		nImageCount = details.capabilities.maxImageCount;
	}

	/* Swap chain create info */
	VkSwapchainCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.minImageCount = nImageCount;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.surface = this->m_surface;

	QueueFamilyIndices indices = this->FindQueueFamilies(this->m_physicalDevice);
	uint32_t queueIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	/* Check if the queue family is not the same */
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueIndices;
	}
	else {
		/* If same queue families, use exclusive mode */
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	
	if (vkCreateSwapchainKHR(this->m_device, &createInfo, nullptr, &this->m_sc) != VK_SUCCESS) {
		spdlog::error("Failed to create the swap chain");
		throw std::runtime_error("Failed to create the swap chain");
		return;
	}

	spdlog::debug("Swap chain created");

	uint32_t nScImageCount;
	vkGetSwapchainImagesKHR(this->m_device, this->m_sc, &nScImageCount, nullptr);
	this->m_scImages.resize(nScImageCount);
	vkGetSwapchainImagesKHR(this->m_device, this->m_sc, &nScImageCount, this->m_scImages.data());	

	this->m_surfaceFormat = format.format;
	this->m_scExtent = extent;
}

/* 
	Creation of our image views 
*/
void VulkanRenderer::CreateImageViews() {
	/* We'll have same amount of image views as swap images */
	this->m_imageViews.resize(this->m_scImages.size());

	/* Initialize each of the image views */
	for (int i = 0; i < this->m_imageViews.size(); i++) {
		VkImageView imageView = this->CreateImageView(this->m_scImages[i], this->m_surfaceFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		this->m_imageViews[i] = imageView;
	}

	spdlog::debug("CreateImageViews: All image views created");
}

/* 
	Creation of our render pass with its attachments and subpasses
*/
void VulkanRenderer::CreateRenderPass() {
	/* Get multisample count */
	VkSampleCountFlagBits multisampleCount = this->GetMaxUsableSampleCount();
	this->m_multisampleCount = multisampleCount;

	/* Color attachment description */
	VkAttachmentDescription colorDesc = { };
	colorDesc.format = this->m_surfaceFormat;
	colorDesc.samples = this->m_multisampleCount;
	colorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Reference to our color attachment */
	VkAttachmentReference colorReference = { };
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Depth attachment description */
	VkAttachmentDescription depthDesc = { };
	depthDesc.format = FindDepthFormat();
	depthDesc.samples = this->m_multisampleCount;
	depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* Reference to our depth attachment */
	VkAttachmentReference depthReference = { };
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* Our color resolve attachment description */
	VkAttachmentDescription colorResolveDesc = { };
	colorResolveDesc.format = this->m_surfaceFormat;
	colorResolveDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorResolveDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorResolveDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolveDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorResolveDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	/* Reference to our color resolve attachment */
	VkAttachmentReference colorResolveReference = { };
	colorResolveReference.attachment = 2;
	colorResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Our sub-pass description */
	VkSubpassDescription subPass = { };
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorReference;
	subPass.pResolveAttachments = &colorResolveReference;
	subPass.pDepthStencilAttachment = &depthReference;
	
	/* Our sub-pass dependency */
	VkSubpassDependency dependency = { };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstSubpass = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	/* Store the descriptors on arrays */
	VkAttachmentDescription attachments[] = { colorDesc, depthDesc, colorResolveDesc };
	VkSubpassDescription subPasses[] = { subPass };
	VkSubpassDependency dependencies[] = { dependency };

	/* Render pass creation info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 3;
	createInfo.dependencyCount = 1;
	createInfo.subpassCount = 1;
	createInfo.pAttachments = attachments;
	createInfo.pDependencies = dependencies;
	createInfo.pSubpasses = subPasses;

	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_renderPass) != VK_SUCCESS) {
		spdlog::error("CreateRenderPass: Failed creating the render pass");
		throw std::runtime_error("CreateRenderPass: Failed creating the render pass");
		return;
	}

	spdlog::debug("CreateRenderPass: Render pass created");
}

/* Command pool creation */
void VulkanRenderer::CreateCommandPool() {
	QueueFamilyIndices indices = FindQueueFamilies(this->m_physicalDevice);

	VkCommandPoolCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = indices.graphicsFamily.value();
	if (vkCreateCommandPool(this->m_device, &createInfo, nullptr, &this->m_commandPool) != VK_SUCCESS) {
		spdlog::error("CreateCommandPool: Error creating command pool");
		throw std::runtime_error("CreateCommandPool: Error creating command pool");
		return;
	}

	spdlog::debug("CreateCommandPool: Command pool created");
}

/* 
	Create an additional image per frame
		Notes:
			- We do this because the swapchain doesn't directly support multisampling.
			  So, we create an additional image and resolve to the final swapchain image.
*/
void VulkanRenderer::CreateColorResources() {
	/* Get format  */
	VkFormat colorFormat = this->m_surfaceFormat;

	spdlog::debug("MSAA Samples available: {0}", static_cast<int>(this->m_multisampleCount));
	
	VkImage colorImage = nullptr;
	VkDeviceMemory imageMemory = nullptr;

	/*
		Creation of our image

		VkImageUsageFlagBits:
			- VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT: Specifies that implementations may support
			  using memory allocations with the VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT to back an image
			  with this usage.
			- VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT: Specifies that the image can be used to create a VkImageView
			  suitable for occupying a VkDescriptorSet slot of type VK_DESCRIPTOR_TYPE_STORAGE_IMAGE

	*/
	uint32_t nColorImageSize = this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		colorFormat,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		colorImage,
		imageMemory
	);

	this->TransitionImageLayout(colorImage, this->m_surfaceFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	/* Creation of our image view */
	VkImageView imageView = this->CreateImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

	this->m_colorImage = colorImage;
	this->m_colorImageMemory = imageMemory;
	this->m_colorImageView = imageView;

	spdlog::debug("VulkanRenderer::CreateColorResources: Color resources created");
}

/* Depth resources creation */
void VulkanRenderer::CreateDepthResources() {
	VkFormat depthFormat = this->FindDepthFormat();

	/* Cretion of our depth image */
	VkDeviceMemory depthMemory = nullptr;
	VkImage depthImage = nullptr;

	uint32_t nDepthImageSize = this->CreateImage(
		this->m_scExtent.width, this->m_scExtent.height,
		depthFormat,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthMemory
	);

	/* Creation of our depth image view */
	VkImageView imageView = this->CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	this->TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	this->m_depthImage = depthImage;
	this->m_depthImageView = imageView;

	spdlog::debug("VulkanRenderer::CreateDepthResources: Depth resources created");
}

/*
	Create a frame buffer per each image view
*/
void VulkanRenderer::CreateFrameBuffers() {
	/* We'll have same frame buffers as image views */
	this->m_frameBuffers.resize(this->m_imageViews.size());

	/* For each image view, create a frame buffer */
	for (uint32_t i = 0; i < this->m_frameBuffers.size(); i++) {
		VkImageView attachments[] = {
			this->m_colorImageView,
			this->m_depthImageView,
			this->m_imageViews[i]
		};

		/* Our frame buffer creation info */
		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.attachmentCount = 3;
		createInfo.pAttachments = attachments;
		createInfo.renderPass = this->m_renderPass;
		createInfo.layers = 1;
		createInfo.width = this->m_scExtent.width;
		createInfo.height = this->m_scExtent.height;

		if (vkCreateFramebuffer(this->m_device, &createInfo, nullptr, &this->m_frameBuffers[i]) != VK_SUCCESS) {
			spdlog::error("CreateFrameBuffer: Error creating frame buffer {0}", i);
			throw std::runtime_error("CreateFrameBuffer: Error creating frame buffer");
			return;
		}
	}

	spdlog::debug("CreateFrameBuffer: Frame buffers created");
}

void VulkanRenderer::CreateCommandBuffer() {
	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(this->m_device, &allocInfo, &this->m_commandBuffer) != VK_SUCCESS) {
		spdlog::error("CreateCommandBuffer: Error allocating command buffer");
		throw std::runtime_error("CreateCommandBuffer: Error allocating command buffer");
		return;
	}

	spdlog::debug("CreateCommandBuffer: Command buffer allocated");
}

/* Creation of our graphics pipeline state */
void VulkanRenderer::CreateGraphicsPipeline() {
	/* Shader compiling */
	std::string sVertShader = this->ReadShader("shader.vert");
	std::vector<uint32_t> vertexShader = this->CompileShader(sVertShader, "shader.vert", shaderc_vertex_shader);

	std::string sFragShader = this->ReadShader("shader.frag");
	std::vector<uint32_t> fragmentShader = this->CompileShader(sFragShader, "shader.frag", shaderc_fragment_shader);

	VkShaderModule vertexModule = this->CreateShaderModule(vertexShader);
	VkShaderModule fragmentModule = this->CreateShaderModule(fragmentShader);

	/* Shader stages */
	VkPipelineShaderStageCreateInfo vertInfo = { };
	vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertInfo.pName = "main";
	vertInfo.module = vertexModule;
	vertInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineShaderStageCreateInfo fragInfo = { };
	fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragInfo.pName = "main";
	fragInfo.module = fragmentModule;
	fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		vertInfo,
		fragInfo
	};

	/* Input binding */
	VkVertexInputBindingDescription bindingDesc = { };
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(Vertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	/* Input attributes */
	VkVertexInputAttributeDescription attribs[3] = {};
	attribs[0].binding = 0;
	attribs[0].location = 0;
	attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[0].offset = offsetof(Vertex, Vertex::position);

	attribs[1].binding = 0;
	attribs[1].location = 1;
	attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[1].offset = offsetof(Vertex, Vertex::normal);

	attribs[2].binding = 0;
	attribs[2].location = 2;
	attribs[2].format = VK_FORMAT_R32G32_SFLOAT;
	attribs[2].offset = offsetof(Vertex, Vertex::texCoord);

	/* 
		Define our dynamic states 
		This allow changing our viewport and scissor in runtime
		With vkCmdSetScissor and vkCmdSetViewport.
	*/
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = { };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStates.data();
	dynamicState.dynamicStateCount = 2;

	/*
		Our vertex input state creation info 
		It defines the format of our vertex attributes
	*/
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInfo.vertexAttributeDescriptionCount = 3;
	vertexInfo.pVertexAttributeDescriptions = attribs;

	/* Our input assembly creation info */
	VkPipelineInputAssemblyStateCreateInfo inputInfo = { };
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* Defining our viewport and scissor */
	this->m_viewport = { };
	this->m_viewport.width = this->m_scExtent.width;
	this->m_viewport.height = -static_cast<float>(this->m_scExtent.height); // Flip our fiewport for +Y up 
	this->m_viewport.x = 0;
	this->m_viewport.y = static_cast<float>(this->m_scExtent.height);
	this->m_viewport.minDepth = 0.f;
	this->m_viewport.maxDepth = 1.f;

	this->m_scissor = { };
	this->m_scissor.extent = this->m_scExtent;

	/* Viewport state create info */
	VkPipelineViewportStateCreateInfo viewportInfo = { };
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pViewports = &this->m_viewport;
	viewportInfo.viewportCount = 1;
	viewportInfo.pScissors = &this->m_scissor;
	viewportInfo.scissorCount = 1;

	/* Our rasterizer state creation info */
	VkPipelineRasterizationStateCreateInfo rasterizer = { };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;

	/* Our multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = { };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = this->m_multisampleCount;
	multisampling.sampleShadingEnable = VK_TRUE; // Improves the quality 
	multisampling.minSampleShading = .2f; // Minimum 20% shading per frame

	/* Color blend */
	VkPipelineColorBlendAttachmentState colorBlend = { };
	colorBlend.blendEnable = VK_FALSE;
	colorBlend.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blendState = { };
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.attachmentCount = 1;
	blendState.pAttachments = &colorBlend;
	blendState.logicOpEnable = VK_FALSE;

	/* Pipeline depth stencil state create info */
	VkPipelineDepthStencilStateCreateInfo depthStencil = { };
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	/* Pipeline layout create info */
	VkPipelineLayoutCreateInfo layoutInfo = { };
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pSetLayouts = nullptr;
	layoutInfo.setLayoutCount = 0;

	if (vkCreatePipelineLayout(this->m_device, &layoutInfo, nullptr, &this->m_pipelineLayout) != VK_SUCCESS) {
		spdlog::error("CreateGraphicsPipeline: Error creating pipeline layout");
		throw std::runtime_error("CreateGraphicsPipeline: Error creating pipeline layout");
		return;
	}
	
	spdlog::debug("CreateGraphicsPipeline: Pipeline layout created");

	/* Graphics pipeline creation */
	VkGraphicsPipelineCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pColorBlendState = &blendState;
	createInfo.subpass = 0;
	createInfo.renderPass = this->m_renderPass;
	createInfo.pMultisampleState = &multisampling;
	createInfo.pViewportState = &viewportInfo;
	createInfo.layout = this->m_pipelineLayout;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pInputAssemblyState = &inputInfo;
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pStages = shaderStages;
	createInfo.stageCount = 2;
	createInfo.pDynamicState = &dynamicState;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = -1;
	createInfo.pDepthStencilState = &depthStencil;

	if (vkCreateGraphicsPipelines(this->m_device, nullptr, 1, &createInfo, nullptr, &this->m_pipeline) != VK_SUCCESS) {
		spdlog::error("CreateGraphicsPipeline: Error creating the pipeline");
		throw std::runtime_error("CreateGraphicsPipeline: Error creating the pipeline");
		return;
	}

	spdlog::debug("CreateGraphicsPipeline: Pipeline created");

	/* Cleanup */
	vkDestroyShaderModule(this->m_device, vertexModule, nullptr);
	vkDestroyShaderModule(this->m_device, fragmentModule, nullptr);
}

/* Creation of our sync objects */
void VulkanRenderer::CreateSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo = { };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = { };
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_renderFinished) != VK_SUCCESS ||
		vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_imageAvailable) != VK_SUCCESS ||
		vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_fence) != VK_SUCCESS) {
		spdlog::error("CreateSyncObjects: Failed to create sync objects");
		throw std::runtime_error("CreateSyncObjects: Failed to create sync objects");
		return;
	}

	spdlog::debug("CreateSyncObjects: Sync objects created");
}

void VulkanRenderer::RecordCommandBuffer(uint32_t nImageIndex) {
	/* Begin command buffer */
	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(this->m_commandBuffer, &beginInfo);

	/* Begin render pass */

	/* Color clear value */
	VkClearValue colorClear = { {{0.f, 0.f, 0.f, 1.f}} };

	/* Depth clear value */
	VkClearValue depthClear = { };
	depthClear.depthStencil = { 1.f, 0 }; 
	
	/* Back buffer clear value */
	VkClearValue backBufferClear = { {{0.f, 0.f, 0.f, 1.f}} };

	VkClearValue clearValues[] = { colorClear, depthClear, backBufferClear };

	VkRenderPassBeginInfo renderPassInfo = { };
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = this->m_renderPass;
	renderPassInfo.framebuffer = this->m_frameBuffers[nImageIndex];
	renderPassInfo.renderArea.extent = this->m_scExtent;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.pClearValues = clearValues;
	renderPassInfo.clearValueCount = 3;
	
	vkCmdBeginRenderPass(this->m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	/* Bind pipeline */
	vkCmdBindPipeline(this->m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_pipeline);

	vkCmdSetViewport(this->m_commandBuffer, 0, 1, &this->m_viewport);
	vkCmdSetScissor(this->m_commandBuffer, 0, 1, &this->m_scissor);

	/* Bind triangle vertex buffer and draw it */
	//this->DrawVertexBuffer(this->m_buffer);

	Scene* currentScene = this->m_sceneMgr->GetCurrentScene();

	std::map<std::string, GameObject*> objects = currentScene->GetObjects();

	for (std::pair<std::string, GameObject*> obj : objects) {
		GameObject* pObj = obj.second;

		std::map<std::string, Component*> components = pObj->GetComponents();

		if (components.count("MeshComponent") <= 0) {
			continue;
		}

		Component* meshComponent = components["MeshComponent"];
		Mesh* mesh = dynamic_cast<Mesh*>(meshComponent);

		if (!mesh) {
			continue;
		}

		std::map<uint32_t, GPUBuffer*> vertices = mesh->GetVBOs();

		for (std::pair<uint32_t, GPUBuffer*> vertex : vertices) {
			this->DrawVertexBuffer(vertex.second);
		}
	}

	vkCmdEndRenderPass(this->m_commandBuffer);
	vkEndCommandBuffer(this->m_commandBuffer);
}

/* 
	Choose for the best swap surface format

	If there is a format that is B8G8R8A8_SRGB and its color space is SRGB_NONLINEAR, select that
*/
VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
	for (const VkSurfaceFormatKHR& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	return formats[0];
}

/*
	Choose the best swap present mode

	If there is a MAILBOX present mode, select it. Else, FIFO_KHR
*/
VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) {
	for (const VkPresentModeKHR& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}


/* 
	Choose swap extent
		If capabilities extent is not valid, create one.
		(Inside the limits)
*/
VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

		extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}
}

/*
	Read our shader file
*/
std::string VulkanRenderer::ReadShader(const std::string& sFile) {
	std::ifstream file(sFile);
	if (!file.is_open()) {
		spdlog::error("Couldn't open shader {0}", sFile);
		throw std::runtime_error("ReadShader: Couldn't open shader");
		return "";
	}

	std::stringstream buff;
	buff << file.rdbuf();
	return buff.str();
}

/*
	Compile our shader to Spir-V code with https://github.com/google/shaderc
*/
std::vector<uint32_t> VulkanRenderer::CompileShader(std::string shader, std::string filename, shaderc_shader_kind kind) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shader, kind, filename.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		spdlog::error("CompileShader: {0}", result.GetErrorMessage());
		throw std::runtime_error("CompileShader: Error compiling shader");
		return std::vector<uint32_t>();
	}

	spdlog::debug("CompileShader: Shader {0} compiled", filename);

	std::vector<uint32_t> shaderCode(result.cbegin(), result.cend());
	return shaderCode;
}

/* Create a shader module from our shader code */
VkShaderModule VulkanRenderer::CreateShaderModule(std::vector<uint32_t>& shaderCode) {
	VkShaderModuleCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
	createInfo.pCode = shaderCode.data();
	
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(this->m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		spdlog::error("CreateShaderModule: Error creating shader module");
		throw std::runtime_error("CreateShaderModule: Error creating shader module");
		
		return nullptr;
	}

	spdlog::debug("CreateShaderModule: Shader module created");

	return shaderModule;
}

/* Find supported format */
VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) {
	/* For each candidate */
	for (VkFormat format : candidates) {
		/* Query physical device format properties */
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(this->m_physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
	}

	spdlog::error("VulkanRenderer::FindSupportedFormat: Failed finding a optimal format candidate");
	throw std::runtime_error("VulkanRenderer::FindSupportedFormat: Failed finding a optimal format candidate");
}

/* Find depth format */
VkFormat VulkanRenderer::FindDepthFormat() {
	return this->FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool VulkanRenderer::HasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/* Create a Vulkan image */
uint32_t VulkanRenderer::CreateImage(
	uint32_t nWidth,
	uint32_t nHeight,
	VkFormat format,
	VkSampleCountFlagBits multisampleCount,
	VkImageTiling tiling,
	VkImageUsageFlags usageFlags,
	VkMemoryPropertyFlags propertyFlags,
	VkImage& image,
	VkDeviceMemory& memory
) {
	/* Image create info */
	VkImageCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.extent.width = nWidth;
	createInfo.extent.height = nHeight;
	createInfo.extent.depth = 1;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.mipLevels = 1;
	createInfo.usage = usageFlags;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.samples = multisampleCount;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.arrayLayers = 1;

	/* Create our image */
	if (vkCreateImage(this->m_device, &createInfo, nullptr, &image) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateImage: Failed creating a VkImage");
		throw std::runtime_error("VulkanRenderer::CreateImage: Failed creating a VkImage");
		return 0;
	}

	/* Query memory requirements */
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(this->m_device, image, &memReqs);

	/* Allocate image memory */
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->FindMemoryType(memReqs.memoryTypeBits, propertyFlags);

	if (vkAllocateMemory(this->m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateImage: Failed allocating image memory");
		throw std::runtime_error("VulkanRenderer::CreateImage: Failed allocating image memory");
		return 0;
	}

	/* Bind image memory */
	if (vkBindImageMemory(this->m_device, image, memory, 0) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateImage: Failed binding image memory");
		throw std::runtime_error("VulkanRenderer::CreateImage: Failed binding image memory");
		return 0;
	}

	return static_cast<uint32_t>(memReqs.size);
}

/* Find memory type */
uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	/* Query physical device memory properties */
	VkPhysicalDeviceMemoryProperties memProps = { };
	vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevice, &memProps);

	/* Find for our memory type */
	for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
		/* If the type is the one that we filter out and it has out property flags, return it */
		if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	}

	spdlog::error("FindMemoryType: Error finding memory type");
	throw std::runtime_error("FindMemoryType: Error finding memory type");
}

/* Creation of a vertex buffer */
GPUBuffer* VulkanRenderer::CreateVertexBuffer(const std::vector<Vertex>& vertices) {
	/* Buffer create info */
	VkBufferCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.size = vertices.size() * sizeof(Vertex);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	 
	/* Creation of our buffer. If fails, throw error and log it */
	VkBuffer buffer = nullptr;
	if (vkCreateBuffer(this->m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
		spdlog::error("CraeteVertexBuffer: Failed creating vertex buffer");
		throw std::runtime_error("CreateVertexBuffer: Failed creating vertex buffer");
		return nullptr;
	}

	spdlog::debug("CreateVertexBuffer: Buffer created for: {0} bytes", vertices.size() * sizeof(Vertex));

	/* Query memory requirements */
	VkMemoryRequirements memReqs = { };
	vkGetBufferMemoryRequirements(this->m_device, buffer, &memReqs);

	/* 
		Define our memory allocation info 
			VkMemoryPropertyFlagBits ref: https://registry.khronos.org/vulkan/specs/latest/man/html/VkMemoryPropertyFlagBits.html

		We'll be using HOST_VISIBLE and HOST_COHERENT:
			HOST_VISIBLE: Specifies that this resource is visible from our device
			HOST_COHERENT: Specifies that the device accesses to this resource are automatically made available and visible
	*/
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	/* Creation of our device memory object */
	VkDeviceMemory memory;
	if (vkAllocateMemory(this->m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		spdlog::error("CreateVertexBuffer: Fialed to allocate our vertex memory");
		throw std::runtime_error("CreateVertexBuffer: Fialed to allocate our vertex memory");
		return nullptr;
	}

	
	/* Map our memory for copying our vertices */
	void* pData = nullptr;
	vkMapMemory(this->m_device, memory, 0, VK_WHOLE_SIZE, 0, &pData);
	memcpy(pData, vertices.data(), vertices.size() * sizeof(Vertex));
	vkUnmapMemory(this->m_device, memory);

	/* Bind our buffer to our device memory */
	if (vkBindBufferMemory(this->m_device, buffer, memory, 0) != VK_SUCCESS) {
		spdlog::error("CreateVertexBuffer: Failed binding buffer memory");
		throw std::runtime_error("CreateVertexBuffer: Failed binding buffer memory");
		return nullptr;
	}

	spdlog::debug("CreateVertexBuffer: Buffer memory binded");

	VulkanBuffer* retBuff = new VulkanBuffer(this->m_device, this->m_physicalDevice, buffer, memory, vertices.size(), EBufferType::VERTEX_BUFFER);
	return retBuff;
}

/*
	Create a CPU visible staging buffer and copy the given data to it.
	This buffer is intended for transferring texture or GPU resource data to GPU memory.
*/
GPUBuffer* VulkanRenderer::CreateStagingBuffer(void* pData, uint32_t nSize) {
	VkBuffer buffer = nullptr;
	VkDeviceMemory memory = nullptr;

	/* Our staging buffer create info */
	VkBufferCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = nSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Staging (Only copy)
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(this->m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateStagingBuffer: Failed creating staging buffer");
		throw std::runtime_error("VulkanRenderer::CreateStagingBuffer: Failed creating staging buffer");
		return nullptr;
	}

	/* Get memory requirements */
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(this->m_device, buffer, &memReqs);

	/* Memory allocate info */
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(this->m_device, &allocInfo, nullptr, &memory);



	/* Map our memory for copying our data */
	void* pMap = nullptr;
	vkMapMemory(this->m_device, memory, 0, memReqs.size, 0, &pMap);
	memcpy(pMap, pData, nSize);
	vkUnmapMemory(this->m_device, memory);

	/* Bind our buffer to our device memory */
	if (vkBindBufferMemory(this->m_device, buffer, memory, 0) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateStagingBuffer: Failed binding buffer memory");
		throw std::runtime_error("VulkanRenderer::CreateStagingBuffer: Failed binding buffer memory");
		return nullptr;
	}

	VulkanBuffer* retBuff = new VulkanBuffer(this->m_device, this->m_physicalDevice, buffer, memory, nSize, EBufferType::STAGING_BUFFER);
	return retBuff;
}

/*
	Create a VkImage from our staging buffer
*/
GPUTexture* VulkanRenderer::CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) {
	VkBuffer vkBuffer = nullptr;
	VulkanBuffer* buffer = dynamic_cast<VulkanBuffer*>(pBuffer);

	if (!buffer) {
		spdlog::error("VulkanRenderer::CreateTexture: Specified GPUBuffer is can't be casted to VulkanBuffer");
		throw std::runtime_error("VulkanRenderer::CreateTexture: Specified GPUBuffer is can't be casted to VulkanBuffer");
		return nullptr;
	}

	vkBuffer = buffer->GetBuffer();

	/* Convert our GPUFormat to a VkFormat */
	VkFormat vkFormat = ToVkFormat(format);

	/*
		Creation of our VkImage
			Notes:
			- VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
				Indicates that the memory is located in device local memory,
				providing the highest performance for GPU access.

			- VK_IMAGE_USAGE_TRANSFER_DST_BIT:
				Indicates that the image can be used as the destination of
				a transfer command

			- VK_IMAGE_USAGE_SAMPLED_BIT:
				Indicates that the image can be used to create a
				VkImageView for ocuppying VkDescriptorSet either of
				type VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE or 
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER and be sampled by
				a shader
			
			References:
				- VkImageUsageFlagsBits: 
					https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageUsageFlagBits.html
				- VkMemoryPropertyFlagBits:
					https://registry.khronos.org/vulkan/specs/latest/man/html/VkMemoryPropertyFlagBits.html
	*/
	VkDeviceMemory imageMemory = nullptr;
	VkImage image = nullptr;
	uint32_t nImageSize = this->CreateImage(
		nWidth, nHeight,
		vkFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image,
		imageMemory);

	this->TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->CopyBufferToImage(vkBuffer, image, nWidth, nHeight);

	this->TransitionImageLayout(image, vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageView imageView = this->CreateImageView(image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	VkSampler sampler = this->CreateSampler();

	VulkanTexture* texture = new VulkanTexture(this->m_device, this->m_physicalDevice, image, imageMemory, nImageSize, imageView, sampler);

	return texture;
}

/* Creates a vulkan image view */
VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;

	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	
	/* How to interpret texel components (normally default) */
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	/* Subresource range: Which image part is going to be used */
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(this->m_device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateImageView: Failed creating image view");
		throw std::runtime_error("VulkanRenderer::CreateImageView: Failed creating image view");
		return nullptr;
	}

	return imageView;
}

/* Creates a sampler */
VkSampler VulkanRenderer::CreateSampler() {
	/* Our sampler create info */
	VkSamplerCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;

	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = 16.f;

	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	createInfo.unnormalizedCoordinates = VK_FALSE;

	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.f;
	createInfo.minLod = 0.f;
	createInfo.maxLod = 0.f;

	/* Creation of our sampler */
	VkSampler sampler;
	if (vkCreateSampler(this->m_device, &createInfo, nullptr, &sampler) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateSampler: Failed creating sampler");
		throw std::runtime_error("VulkanRenderer::CreateSampler: Failed creating sampler");
		return nullptr;
	}

	return sampler;
}

/* Copy our buffer to a VkImage */
void VulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t nWidth, uint32_t nHeight) {
	VkCommandBuffer commandBuffer = this->BeginSingleTimeCommandBuffer();

	VkBufferImageCopy region = { };
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { nWidth, nHeight, 1 };

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region
	);

	this->EndSingleTimeCommandBuffer(commandBuffer);
}

/* Bind our vertex buffer */
bool VulkanRenderer::DrawVertexBuffer(GPUBuffer* buffer) {
	/* First of all, check if the specified buffer is a VulkanBuffer */
	if (dynamic_cast<VulkanBuffer*>(buffer) == nullptr) {
		spdlog::error("BindVertexBuffer: Specified GPUBuffer is not a Vulkan buffer");
		throw std::runtime_error("BindVertexBuffer: Specified GPUBuffer is not a Vulkan buffer");
		return false;
	}

	VulkanBuffer* vkBuff = dynamic_cast<VulkanBuffer*>(buffer);
	
	/* Get our VkBuffer, VkDeviceMemory and our buffer size */
	VkBuffer buff = vkBuff->GetBuffer();
	VkDeviceMemory memory = vkBuff->GetMemory();
	uint32_t nSize = vkBuff->GetSize();

	/* Bind our vertex buffer and draw it */
	VkBuffer vertexBuffers[] = { buff };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(this->m_commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdDraw(this->m_commandBuffer, nSize, 1, 0, 0);

	return true;
}

/* Get the max usable sample count depending on the physical device properties */
VkSampleCountFlagBits VulkanRenderer::GetMaxUsableSampleCount() {
	/* Get physical device properties */
	VkPhysicalDeviceProperties devProps;
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &devProps);

	/*
		Determine the supported levels of multisampling simultaneously by color and depth.
		Vulkan needs that both attachments use the same sample count on the render pass.
	*/
	VkSampleCountFlags count = devProps.limits.framebufferColorSampleCounts & devProps.limits.framebufferDepthSampleCounts;

	if (count & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	if (count & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	if (count & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	if (count & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
	if (count & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
	if (count & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

/* Transition image layout to a new one */
void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = this->BeginSingleTimeCommandBuffer();

	/* Create our barrier */
	VkImageMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	/* Barrier subresource range definition */
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	/* 
		VkPipelineStageFlags is a bitmask type for setting a mask of zero or more VkPipelineStageFlagBits
			Reference:	https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineStageFlags.html
			Bitmask reference: https://registry.khronos.org/vulkan/specs/latest/man/html/VkPipelineStageFlagBits.html
	*/
	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	/* From UNDEFINED to TRANSFER_DST */
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	/* From TRANSFER_DST to SHADER_READ_ONLY */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* From UNDEFINED to DEPTH_STENCIL_ATTACHMENT */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (this->HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	/* From UNDEFINED to COLOR_ATTACHMENT */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else {
		spdlog::error("VulkanRenderer::TransitionImageLayout: Unsupported layout transition");
		throw std::runtime_error("VulkanRenderer::TransitionImageLayout: Unsupported layout transition");
		return;
	}

	/* Make transition */
	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	this->EndSingleTimeCommandBuffer(commandBuffer);
}

/* Begins a single time vulkan command buffer */
VkCommandBuffer VulkanRenderer::BeginSingleTimeCommandBuffer() {
	/* Allocate our command buffer */
	VkCommandBufferAllocateInfo cmdBuffInfo = { };
	cmdBuffInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBuffInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBuffInfo.commandPool = this->m_commandPool;
	cmdBuffInfo.commandBufferCount = 1; // Only one command buffer

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(this->m_device, &cmdBuffInfo, &commandBuffer) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::BeginSingleTimeCommandBuffer: Failed allocating command buffer");
		throw std::runtime_error("VulkanRenderer::BeginSingleTimeCommandBuffer: Failed allocating command buffer");
		return nullptr;
	}

	/* Begin our single time command buffer */
	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

/* Ends a single time command buffer and submits to the graphics queue */
void VulkanRenderer::EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	/* Submit our command buffer to our graphics queue */
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(this->m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(this->m_graphicsQueue);

	/* Free our command buffer */
	vkFreeCommandBuffers(this->m_device, this->m_commandPool, 1, &commandBuffer);
}

/* Check if the physical device is suitable */
bool VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = this->FindQueueFamilies(device); /* Fetch queue family indices */

	bool bExtensionsSupported = this->CheckDeviceExtensionSupport(device); /* Check if device extensions are supported and store it */

	bool bSwapChainAdequate = false;
	if (bExtensionsSupported) {
		// If formats or present modes are empty, then, will be false.
		SwapChainSupportDetails swapChainSupport = this->QuerySwapChainSupport(device);	
		bSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsComplete() && bExtensionsSupported && bSwapChainAdequate;
}

/* Find queue families for physical device */
QueueFamilyIndices VulkanRenderer::FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	/* Enumerate physical device queue family properties */
	uint32_t nQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueueFamilyCount, nullptr);

	/* Store physical device queue family properties on a vector */
	std::vector<VkQueueFamilyProperties> queueFamilies(nQueueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilies) {
		/* Check if queue family supports graphics commands  */
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		/* Check if queue family supports presentation to the surface  */
		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->m_surface, &bPresentSupport);

		if (bPresentSupport) {
			indices.presentFamily = i;
		}

		/* If indices is complete, break the cycle */
		if (indices.IsComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

/* Check if physical device supports device extensions */
bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	/* Enumerate device extensions */
	uint32_t nExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &nExtensionCount, nullptr);

	/* Store device extensions to a vector */
	std::vector<VkExtensionProperties> extensions(nExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &nExtensionCount, extensions.data());

	/* Make a copy of the 'deviceExtensions' vector to a set */
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	/* 
		If extension is on required extensions list, remove it.
		Required extensions set is empty? Then, all the extensions are supported.
	*/
	for (const VkExtensionProperties& extension : extensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

/* Check if device supports swap chain */
SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	/* Get physical device surface capabilities and store at swap chain support details' capabilities field */
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->m_surface, &details.capabilities);

	/* Enumerate physical device surface formats */
	uint32_t nFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->m_surface, &nFormatCount, nullptr);

	/* If format count not zero, store on the formats field from swap chain support details */
	if (nFormatCount != 0) {
		details.formats.resize(nFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->m_surface, &nFormatCount, details.formats.data());
	}

	/* Enumerate physical device surface present modes */
	uint32_t nPresentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->m_surface, &nPresentModeCount, nullptr);
	
	/* If present mode count not zero, store on the present modes field from swap chain support details */
	if (nPresentModeCount != 0) {
		details.presentModes.resize(nPresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->m_surface, &nPresentModeCount, details.presentModes.data());
	}

	return details;
}

/* Get required extensions */
std::vector<const char*> VulkanRenderer::GetRequiredExtensions() {
	/* Enum GLFW extensions */
	uint32_t nGlfwExtensions = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&nGlfwExtensions);

	/* Store GLFW extensions on a vector */
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + nGlfwExtensions);

	if (this->m_bEnableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}


/* Populates a create info for our debug messenger */
void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = this->DebugCallback;
	createInfo.pUserData = nullptr;
}

/* Check for validation layers support */
bool VulkanRenderer::CheckValidationLayersSupport() {
	/* Enumerate instance layer properties */
	uint32_t nLayerCount;
	vkEnumerateInstanceLayerProperties(&nLayerCount, nullptr);

	/* Store instance layer properties on a vector */
	std::vector<VkLayerProperties> availableLayers(nLayerCount);
	vkEnumerateInstanceLayerProperties(&nLayerCount, availableLayers.data());

	/* Check if validation layer is available */
	for (const char* layerName : validationLayers) {
		bool bLayerFound = false;
		
		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				bLayerFound = true;
				break;
			}
		}

		if (!bLayerFound)
			return false;
	}

	return true;
}

/* Debug messenger callback */
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
	VkDebugUtilsMessageTypeFlagsEXT msgType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCbData,
	void* pvUserData
) {
	switch (msgSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		spdlog::debug("Validation layer: {}", pCbData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		spdlog::warn("Validation layer: {}", pCbData->pMessage);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		spdlog::warn("Validation layer: {}", pCbData->pMessage);
		break;
	}

	return VK_FALSE;
}

/* Manual extension function loading (vkCreateDebugUtilsMessengerEXT) */
VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (fn != nullptr) {
		return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/* Manual extension function loading (vkDestroyDebugUtilsMessengerEXT) */
void VulkanRenderer::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator
) {
	PFN_vkDestroyDebugUtilsMessengerEXT fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (fn != nullptr) {
		fn(instance, debugMessenger, pAllocator);
	}
}

void VulkanRenderer::Update() {
	Renderer::Update();

	vkWaitForFences(this->m_device, 1, &this->m_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(this->m_device, 1, &this->m_fence);

	uint32_t nImageIndex;
	vkAcquireNextImageKHR(this->m_device, this->m_sc, UINT64_MAX, this->m_imageAvailable, VK_NULL_HANDLE, &nImageIndex);

	this->RecordCommandBuffer(nImageIndex);
	
	VkSemaphore waitSemaphores[] = { this->m_imageAvailable };
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSemaphore signalSemaphores[] = { this->m_renderFinished };

	/* Setup our submit info */
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	
	submitInfo.pCommandBuffers = &this->m_commandBuffer;
	submitInfo.commandBufferCount = 1;

	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.signalSemaphoreCount = 1;

	if (vkQueueSubmit(this->m_graphicsQueue, 1, &submitInfo, this->m_fence) != VK_SUCCESS) {
		spdlog::error("Update: Failed to queue submit");
		throw std::runtime_error("Update: Failed to queue submit");
		return;
	}
	
	/* Present our image */
	VkSwapchainKHR swapChains[] = { this->m_sc };
	
	VkPresentInfoKHR presentInfo = { };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &nImageIndex;

	vkQueuePresentKHR(this->m_presentQueue, &presentInfo);
}