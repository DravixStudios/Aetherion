#include "Core/Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Scene/SceneManager.h"

#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>

/* Validation layers (hard-coded) */
Vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

/* Device extensions (hard-coded) */
Vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#ifdef __APPLE__
	"VK_KHR_portability_subset"
#endif
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
	Vector<VkSurfaceFormatKHR> formats;
	Vector<VkPresentModeKHR> presentModes;
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

/* Return VkBufferUsageFlagBit depending on the buffer type */
VkBufferUsageFlagBits ToVkBufferUsage(EBufferType bufferType) {
	switch (bufferType) {
		case EBufferType::CONSTANT_BUFFER: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case EBufferType::VERTEX_BUFFER: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		case EBufferType::INDEX_BUFFER: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	};
}

struct PushConstant {
	uint32_t nTextureIndex;
	uint32_t nOrmTextureIndex;
	uint32_t nEmissiveTextureIndex;
};

struct LightPushConstant {
	glm::vec3 cameraPosition;
};

struct SkyboxPushConstant {
	glm::mat4 inverseView;
	glm::mat4 inverseProjection;
	glm::vec3 cameraPosition;
};

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
	this->m_sc = nullptr;
	this->m_scExtent = VkExtent2D();
	this->m_surfaceFormat = VkFormat::VK_FORMAT_UNDEFINED;
	this->m_pipeline = nullptr;
	this->m_pipelineLayout = nullptr;
	this->m_scissor = VkRect2D();
	this->m_viewport = VkViewport();
	this->m_commandPool = nullptr;
	this->m_sceneMgr = nullptr;
	this->m_nImageCount = 0;
	this->m_nMaxDescriptorSetSamplers = 0;
	this->m_nMaxPerStageDescriptorSamplers = 0;
	this->m_nMaxTextures = 0;
	this->m_nCurrentFrameIndex = 0;
}

/* Renderer init method */
void VulkanRenderer::Init() {
	Renderer::Init();

	this->CreateInstance();
	this->SetupDebugMessenger();
	this->CreateSurface();
	this->PickPhysicalDevice();
	this->CheckDescriptorIndexingSupport();
	this->QueryDeviceLimits();
	this->CreateLogicalDevice();
	this->CreateSwapChain();
	this->CreateImageViews();
	this->CreateGeometryRenderPass();
	this->CreateLightingRenderPass();
	this->CreateSkyboxRenderPass();
	this->CreateIrradianceRenderPass();
	this->CreatePrefilterRenderPass();
	this->CreateBRDFRenderPass();
	this->CreateCommandPool();
	this->CreateGBufferResources();
	this->CreateDepthResources();
	this->CreateLightingResources();
	this->CreateCubeMesh();
	this->CreateGBufferFrameBuffer();
	this->CreateLightingFrameBuffer();
	this->CreateSkyboxFrameBuffer();

	/* Test uniform */
	this->m_wvp.World = glm::mat4(1.f);
	this->m_wvp.View = glm::mat4(1.f);
	this->m_wvp.View = glm::translate(this->m_wvp.View, { 0.f, 0.f, 2.f });
	this->m_wvp.View = glm::affineInverse(this->m_wvp.View);
	this->m_wvp.Projection = glm::perspectiveFovRH(glm::radians(70.f), static_cast<float>(this->m_scExtent.width), static_cast<float>(this->m_scExtent.height), .001f, 300.f);

	VulkanRingBuffer* pUrb = new VulkanRingBuffer(this->m_device, this->m_physicalDevice);
	pUrb->Init(2 * 1024 * 1024, 256, this->m_nImageCount);
	this->m_wvpBuff = pUrb;
	/* End Test uniform */

	this->CreateDescriptorSetLayout();
	this->CreateLightingDescriptorSetLayout();
	this->CreateSkyboxDescriptorSetLayout();
	this->CreateDescriptorPool();
	this->CreateLightingDescriptorPool();
	this->CreateSkyboxDescriptorPool();
	this->AllocateDescriptorSets();
	this->AllocateLightingDescriptorSets();
	this->AllocateSkyboxDescriptorSets();
	this->WriteDescriptorSets();
	this->WriteLightDescriptorSets();

	/* Test skybox */
	this->m_skybox = this->CreateCubemap("cubemap.exr", ECubemapLayout::HORIZONTAL_CROSS); // Create a sample skybox cubemap
	this->WriteSkyboxDescriptorSets();
	/* End test skybox */

	this->CreateCommandBuffer();
	this->CreateGBufferPipeline();
	this->CreateLightingPipeline();
	this->CreateSkyboxPipeline();
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
	appInfo.apiVersion = VK_API_VERSION_1_2;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "No name"; /* Actually not making a game lol. We're only setting pEngineName */
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "Aetherion Engine";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	/* Fetch required extensions and size */
	Vector<const char*> extensions = this->GetRequiredExtensions();
#ifdef __APPLE__
	extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
	size_t nExtensionCount = extensions.size();

	spdlog::debug("Required extension count: {0}", nExtensionCount);

	/* Vulkan instance creation info */
	VkInstanceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = nExtensionCount;
	createInfo.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

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
	Vector<VkPhysicalDevice> physicalDevices(nPhysicalDeviceCount);
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

	if (apiMajor < 1 || (apiMajor == 1 && apiMinor < 2)) {
		spdlog::error("PickPhysicalDevice: Selected GPU does not support Vulkan 1.2 minimum (found {}.{})",
			apiMajor, apiMinor);
		throw std::runtime_error("PickPhysicalDevice: Vulkan 1.2 required.");
	}



	spdlog::debug("Selected physical device: {0}", deviceProperties.deviceName);
}

/* Check if device supports decriptor indexing */
void VulkanRenderer::CheckDescriptorIndexingSupport() {
	/* Query number of available extensions */
	uint32_t nExtensionCount;
	vkEnumerateDeviceExtensionProperties(this->m_physicalDevice, nullptr, &nExtensionCount, nullptr);

	/* Query available extensions */
	Vector<VkExtensionProperties> availableExtensions(nExtensionCount);
	vkEnumerateDeviceExtensionProperties(this->m_physicalDevice, nullptr, &nExtensionCount, availableExtensions.data());

	/* Check if descriptor indexing extension is available */
	bool bDescriptorIndexingSupported = false;
 
	for (const VkExtensionProperties extension : availableExtensions) {
		if (strcmp(extension.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
			bDescriptorIndexingSupported = true;
			break;
		}
	}

	if (!bDescriptorIndexingSupported) {
		spdlog::error("VulkanRenderer::CheckDescriptorIndexingSupport: Descriptor indexing is not supported");
		return;
	}

	spdlog::debug("VulkanRenderer::CheckDescriptorIndexSupport: Descriptor indexing supported");

	/* Verify specific features */
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = { };
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

	VkPhysicalDeviceFeatures2 features2 = { };
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &indexingFeatures;

	vkGetPhysicalDeviceFeatures2(this->m_physicalDevice, &features2);

	spdlog::debug("=== Descriptor indexing features ===");
	spdlog::debug("Descriptor binding partially bound: {0}", indexingFeatures.descriptorBindingPartiallyBound ? "Yes" : "No");
	spdlog::debug("Descriptor binding update after bind: {0}", indexingFeatures.descriptorBindingUpdateUnusedWhilePending ? "Yes" : "No");
	spdlog::debug("Descriptor binding varialble descriptor count: {0}", indexingFeatures.descriptorBindingVariableDescriptorCount ? "Yes" : "No");
	spdlog::debug("Runtime descriptor array: {0}", indexingFeatures.runtimeDescriptorArray ? "Yes" : "No");
	spdlog::debug("=== End descriptor indexing features ===");

	if (!indexingFeatures.descriptorBindingPartiallyBound || !indexingFeatures.runtimeDescriptorArray) {
		spdlog::error("VulkanRenderer::CheckDescriptorIndexingSupport: Required descriptor indexing features not available");
	}
}

/* Get GPU limits */
void VulkanRenderer::QueryDeviceLimits() {
	/* Get physical device properties */
	VkPhysicalDeviceProperties devProps;
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &devProps);

	VkPhysicalDeviceLimits limits = devProps.limits;

	VkPhysicalDeviceDescriptorIndexingProperties indexingProps = { };
	indexingProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

	VkPhysicalDeviceProperties2 props2 = { };
	props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props2.pNext = &indexingProps;

	vkGetPhysicalDeviceProperties2(this->m_physicalDevice, &props2);

	spdlog::debug("=== GPU Limits ===");
	spdlog::debug("Max sampler allocation count: {0}", limits.maxSamplerAllocationCount);
	spdlog::debug("Max per-stage descriptor samplers: {0}", limits.maxPerStageDescriptorSamplers);
	spdlog::debug("Max descriptor set samplers: {0}", limits.maxDescriptorSetSamplers);
	spdlog::debug("Max descriptor set update after bind samplers: {0}", indexingProps.maxDescriptorSetUpdateAfterBindSamplers);
	spdlog::debug("Max per-stage descriptor update after bind samplers: {0}", indexingProps.maxPerStageDescriptorUpdateAfterBindSamplers);
	spdlog::debug("Max per-stage update after bind resources: {0}", indexingProps.maxPerStageUpdateAfterBindResources);
	spdlog::debug("=== End GPU Limits ===");

	this->m_nMaxDescriptorSetSamplers = std::min(
		limits.maxDescriptorSetSamplers,
		indexingProps.maxDescriptorSetUpdateAfterBindSamplers
	);

	this->m_nMaxPerStageDescriptorSamplers = std::min(
		limits.maxPerStageDescriptorSamplers,
		indexingProps.maxPerStageDescriptorUpdateAfterBindSamplers
	);

	this->m_nMaxTextures = std::min(
		this->m_nMaxDescriptorSetSamplers,
		static_cast<uint32_t>(32768)
	);

	spdlog::debug("VulkanRenderer::QueryDeviceLimits: Max textures: {0}", this->m_nMaxTextures);
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

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

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

	/* Enable descriptor indexing */
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = { };
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	indexingFeatures.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
	indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;
	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	indexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;

	/* Physical device features */
	VkPhysicalDeviceFeatures deviceFeatures = { };
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkPhysicalDeviceFeatures2 features2 = { };
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.features = deviceFeatures;
	features2.pNext = &indexingFeatures;

	/* Logical device create info */
	VkDeviceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pNext = &features2;

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

	this->m_nImageCount = nImageCount;

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
	Create our geometry render pass 
*/
void VulkanRenderer::CreateGeometryRenderPass() {
	/* Get multisample count */
	VkSampleCountFlagBits multisampleCount = this->GetMaxUsableSampleCount();
	this->m_multisampleCount = multisampleCount;

	/* Attachment 0: Base color - RGBA16_FLOAT */
	VkAttachmentDescription2 colorAttachment = { };
	colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	colorAttachment.samples = this->m_multisampleCount;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 1: Normal - RGBA16_SFLOAT */
	VkAttachmentDescription2 normalAttachment = { };
	normalAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	normalAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalAttachment.samples = this->m_multisampleCount;
	normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 2: ORM - RGBA16_SFLOAT */
	VkAttachmentDescription2 ormAttachment = { };
	ormAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	ormAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	ormAttachment.samples = this->m_multisampleCount;
	ormAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ormAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ormAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ormAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ormAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ormAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 3: Emissive - RGBA16_SFLOAT */
	VkAttachmentDescription2 emissiveAttachment = { };
	emissiveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	emissiveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	emissiveAttachment.samples = this->m_multisampleCount;
	emissiveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	emissiveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	emissiveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	emissiveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	emissiveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 4: Position - RGBA16_SFLOAT */
	VkAttachmentDescription2 positionAttachment = { };
	positionAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	positionAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	positionAttachment.samples = this->m_multisampleCount;
	positionAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	positionAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 5: Depth */
	VkAttachmentDescription2 depthAttachment = { };
	depthAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	depthAttachment.format = this->FindDepthFormat();
	depthAttachment.samples = this->m_multisampleCount;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 6: Base color Resolve - RGBA16_SFLOAT */
	VkAttachmentDescription2 colorResolveAttachment = { };
	colorResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	colorResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 7: Normal Resolve - RGBA16_SFLOAT */
	VkAttachmentDescription2 normalResolveAttachment = { };
	normalResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	normalResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	normalResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	normalResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	normalResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	normalResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	normalResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	normalResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 8: ORM Resolve - RGBA16_SFLOAT */
	VkAttachmentDescription2 ormResolveAttachment = { };
	ormResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	ormResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	ormResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ormResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ormResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ormResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ormResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ormResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ormResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 9: Emissive Resolve - RGBA16_SFLOAT */
	VkAttachmentDescription2 emissiveResolveAttachment = { };
	emissiveResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	emissiveResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	emissiveResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	emissiveResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	emissiveResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	emissiveResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	emissiveResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	emissiveResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 10: Position Resolve - RGBA16_SFLOAT */
	VkAttachmentDescription2 positionResolveAttachment = { };
	positionResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	positionResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	positionResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	positionResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	positionResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	positionResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	positionResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	positionResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	positionResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Attachment 11: Depth Resolve */
	VkAttachmentDescription2 depthResolveAttachment = { };
	depthResolveAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	depthResolveAttachment.format = this->FindDepthFormat();
	depthResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Base color attachment reference */
	VkAttachmentReference2 colorRef = { };
	colorRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Normal attachment reference */
	VkAttachmentReference2 normalRef = { };
	normalRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	normalRef.attachment = 1;
	normalRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* ORM attachment reference */
	VkAttachmentReference2 ormRef = { };
	ormRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	ormRef.attachment = 2;
	ormRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Emissive attachment reference */
	VkAttachmentReference2 emissiveRef = { };
	emissiveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	emissiveRef.attachment = 3;
	emissiveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Position attachment reference */
	VkAttachmentReference2 positionRef = { };
	positionRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	positionRef.attachment = 4;
	positionRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Depth attachment reference */
	VkAttachmentReference2 depthRef = { };
	depthRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	depthRef.attachment = 5;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/* Base color resolve attachment reference */
	VkAttachmentReference2 colorResolveRef = { };
	colorResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	colorResolveRef.attachment = 6;
	colorResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Normal resolve attachment reference */
	VkAttachmentReference2 normalResolveRef = { };
	normalResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	normalResolveRef.attachment = 7;
	normalResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* ORM resolve attachment reference */
	VkAttachmentReference2 ormResolveRef = { };
	ormResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	ormResolveRef.attachment = 8;
	ormResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Emissive resolve attachment reference */
	VkAttachmentReference2 emissiveResolveRef = { };
	emissiveResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	emissiveResolveRef.attachment = 9;
	emissiveResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Position resolve attachment reference */
	VkAttachmentReference2 positionResolveRef = { };
	positionResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	positionResolveRef.attachment = 10;
	positionResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Depth resolve attachment reference */
	VkAttachmentReference2 depthResolveRef = { };
	depthResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	depthResolveRef.attachment = 11;
	depthResolveRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference2 colorRefs[] = { colorRef, normalRef, ormRef, emissiveRef, positionRef, depthRef };
	VkAttachmentReference2 resolveRefs[] = { colorResolveRef, normalResolveRef, ormResolveRef, emissiveResolveRef, positionResolveRef };
	
	VkAttachmentDescription2 attachments[] = {
		colorAttachment,
		normalAttachment,
		ormAttachment,
		emissiveAttachment,
		positionAttachment,
		depthAttachment,
		colorResolveAttachment,
		normalResolveAttachment,
		ormResolveAttachment,
		emissiveResolveAttachment,
		positionResolveAttachment,
		depthResolveAttachment
	};

	/* Depth stencil resolve */
	VkSubpassDescriptionDepthStencilResolve depthStencilResolve = { };
	depthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
	depthStencilResolve.pDepthStencilResolveAttachment = &depthResolveRef;
	depthStencilResolve.pNext = nullptr;
	depthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	depthStencilResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;

	/* Subpass description */
	VkSubpassDescription2 subpass = { };
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 5;
	subpass.pColorAttachments = colorRefs;
	subpass.pDepthStencilAttachment = &depthRef; // MSAA Depth
	subpass.pResolveAttachments = resolveRefs;
	subpass.pNext = &depthStencilResolve;

	/* Subpass dependency */
	VkSubpassDependency2 dependency = { };
	dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	/* Render pass create info */
	VkRenderPassCreateInfo2 createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	createInfo.attachmentCount = 12;
	createInfo.pAttachments = attachments;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	/* Create render pass */
	if (vkCreateRenderPass2(this->m_device, &createInfo, nullptr, &this->m_geometryRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateGeometryRenderPass: Failed creating geometry render pass");
		throw std::runtime_error("VulkanRenderer::CreateGeometryRenderPass: Failed creating geometry render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateGeometryRenderPass: Geometry render pass created");
}

/*
	Creation of our lighting render pass (Screen Quad)
*/
void VulkanRenderer::CreateLightingRenderPass() {
	/* Attachment 0: Final color output */
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = this->m_surfaceFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Final color attachment reference */
	VkAttachmentReference colorRef = { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Subpass */
	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	/* Subpass dependency */
	VkSubpassDependency dependency = { };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = 0;

	/* Render pass create info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.pDependencies = &dependency;
	createInfo.dependencyCount = 1;

	/* Create our render pass */
	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_lightingRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateLightingRenderPass: Failed creating lighting render pass");
		throw std::runtime_error("VulkanRenderer::CreateLightingRenderPass: Failed creating lighting render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateLightingRenderPass: Lighting render pass created");
}

/*
	Creation of our skybox render pass (Screen Quad over our lighting frame)
*/
void VulkanRenderer::CreateSkyboxRenderPass() {
	/* Attachment 0: Final color output */
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = this->m_surfaceFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	/* Final color attachment reference */
	VkAttachmentReference colorRef = { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Subpass */
	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	/* Subpass dependency */
	VkSubpassDependency dependency = { };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = 0;

	/* Render pass create info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pAttachments = &colorAttachment;
	createInfo.attachmentCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.subpassCount = 1;
	createInfo.pDependencies = &dependency;
	createInfo.dependencyCount = 1;

	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_skyboxRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateSkyboxRenderPass: Failed creating skybox render pass");
		throw std::runtime_error("VulkanRenderer::CreateSkyboxRenderPass: Failed creating skybox render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateSkyboxRenderPass: Skybox render pass created");
}

/* Create irradiance render pass */
void VulkanRenderer::CreateIrradianceRenderPass() {
	/* Attachment 0: Color attachment */
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Color attachment reference */
	VkAttachmentReference colorRef = { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Subpass description */
	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	/* Render pass create info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_irradianceRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateIrradianceRenderPass: Failed creating irradiance render pass");
		throw std::runtime_error("VulkanRenderer::CreateIrradianceRenderPass: Failed creating irradiance render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateIrradianceRenderPass: Irradiance render pass created");
}

/* Create prefilter render pass (Same as irradiance) */
void VulkanRenderer::CreatePrefilterRenderPass() {
	/* Attachment 0: Color attachment */
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Color attachment reference */
	VkAttachmentReference colorRef = { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Subpass description */
	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	/* Render pass create info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_prefilterRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreatePrefilterRenderPass: Failed creating prefilter render pass");
		throw std::runtime_error("VulkanRenderer::CreatePrefilterRenderPass: Failed creating prefilter render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreatePrefilterRenderPass: Prefilter render pass created");
}

/* Create BRDF render pass */
void VulkanRenderer::CreateBRDFRenderPass() {
	/* Attachment 0: Color attachment */
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = VK_FORMAT_R32G32_SFLOAT; // Only 2 channels (RG)
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	/* Color attachment reference */
	VkAttachmentReference colorRef = { };
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Subpass description */
	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;

	/* Render pass create info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(this->m_device, &createInfo, nullptr, &this->m_brdfRenderPass) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateBRDFRenderPass: Failed creating BRDF render pass");
		throw std::runtime_error("VulkanRenderer::CreateBRDFRenderPass: Failed creating BRDF render pass");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateBRDFRenderPass: BRDF render pass created");
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
	Create our G-Buffer resources
*/
void VulkanRenderer::CreateGBufferResources() {
	VkDeviceMemory colorMemory, normalMemory, ormMemory, emissiveMemory, positionMemory = nullptr;
	VkDeviceMemory colorResolveMemory, normalResolveMemory, ormResolveMemory, emissiveResolveMemory, positionResolveMemory = nullptr;

	/* Base color G-Buffer (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_colorBuffer,
		colorMemory
	);

	/* Normal G-Buffer (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_normalBuffer,
		normalMemory
	);

	/* ORM G-Buffer (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_ormBuffer,
		ormMemory
	);

	/* Emissive G-Buffer (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_emissiveBuffer,
		emissiveMemory
	);

	/* Position G-Buffer (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		this->m_multisampleCount,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_positionBuffer,
		positionMemory
	);

	/* Base color G-Buffer resolve (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_colorResolveBuffer,
		colorResolveMemory
	);

	/* Normal G-Buffer resolve (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_normalResolveBuffer,
		normalResolveMemory
	);

	/* ORM G-Buffer resolve (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_ormResolveBuffer,
		ormResolveMemory
	);

	/* Emissive G-Buffer resolve (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_emissiveResolveBuffer,
		emissiveResolveMemory
	);

	/* Position G-Buffer resolve (RGBA16_SFLOAT) */
	this->CreateImage(
		this->m_scExtent.width,
		this->m_scExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		this->m_positionResolveBuffer,
		positionResolveMemory
	);

	/* Create our G-Buffer image views */
	this->m_colorBuffView = this->CreateImageView(this->m_colorBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_normalBuffView = this->CreateImageView(this->m_normalBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_ormBuffView = this->CreateImageView(this->m_ormBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_emissiveBuffView = this->CreateImageView(this->m_emissiveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_positionBuffView = this->CreateImageView(this->m_positionBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	this->m_colorResolveBuffView = this->CreateImageView(this->m_colorResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_normalResolveBuffView = this->CreateImageView(this->m_normalResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_ormResolveBuffView = this->CreateImageView(this->m_ormResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_emissiveResolveBuffView = this->CreateImageView(this->m_emissiveResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
	this->m_positionResolveBuffView = this->CreateImageView(this->m_positionResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	/* Create our G-Buffer samplers */
	this->m_baseColorSampler = this->CreateSampler();
	this->m_normalSampler = this->CreateSampler();
	this->m_ormSampler = this->CreateSampler();
	this->m_emissiveSampler = this->CreateSampler();
	this->m_positionSampler = this->CreateSampler();

	/* Transition our G-Buffer images */
	this->TransitionImageLayout(this->m_colorBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_normalBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_ormBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_emissiveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_positionBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	this->TransitionImageLayout(this->m_colorResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_normalResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_ormResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_emissiveResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImageLayout(this->m_positionResolveBuffer, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	spdlog::debug("VulkanRenderer::CreateGBufferResources: G-Buffers created");
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
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthMemory
	);

	/* Creation of our depth image view */
	VkImageView imageView = this->CreateImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	this->TransitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	this->m_depthImage = depthImage;
	this->m_depthImageView = imageView;

	VkDeviceMemory depthResolveMemory = nullptr;
	VkImage depthResolveImage = nullptr;

	uint32_t nDepthResolveImageSize = this->CreateImage(
		this->m_scExtent.width, this->m_scExtent.height,
		depthFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthResolveImage,
		depthResolveMemory
	);

	VkImageView depthResolveImageView = this->CreateImageView(depthResolveImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	this->TransitionImageLayout(depthResolveImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	this->m_depthResolveImage = depthResolveImage;
	this->m_depthResolveImageView = depthResolveImageView;

	this->m_depthSampler = this->CreateSampler();

	spdlog::debug("VulkanRenderer::CreateDepthResources: Depth resources created");
}

/* Create lighting resources */
void VulkanRenderer::CreateLightingResources() {
	/* Screen quad vertices */
	Vector<ScreenQuadVertex> vertices = {
		{ { -1.f, -1.f, 0.f }, { 0.f, 0.f  }}, // BL
		{ { -1.f, 1.f, 0.f }, { 0.f, 1.f  }}, // TL
		{ { 1.f, 1.f, 0.f }, { 1.f, 1.f  }}, // TR
		{ { 1.f, -1.f, 0.f }, { 1.f, 0.f  }}, // BR
	};

	/* Screen quad indices */
	Vector<uint16_t> indices = {
		0, 1, 2,
		0, 2, 3
	};

	GPUBuffer* vbo = this->CreateVertexBuffer(vertices);
	spdlog::debug("VulkanRenderer::CreateLightingResources: ScreenQuad Vertex Buffer object created");
	GPUBuffer* ibo = this->CreateIndexBuffer(indices);
	spdlog::debug("VulkanRenderer::CreateLightingResources: ScreenQuad Index Buffer object created");

	this->m_sqVBO = vbo;
	this->m_sqIBO = ibo;
}

/* Creates a cube mesh for IBL */
void VulkanRenderer::CreateCubeMesh() {
	/* Vertices of a centered cube */
	Vector<glm::vec3> vertices = {
		// Back face
		{ -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, 1.f, -1.f }, { -1.f, 1.f, -1.f },
		// Front face
		{ -1.f, -1.f, 1.f }, { 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, { -1.f, 1.f, 1.f  },
		// Left face
		{ -1.f, -1.f, -1.f }, { -1.f, -1.f, 1.f }, { -1.f, 1.f, 1.f }, { -1.f, 1.f, -1.f },
		// Right face
		{ 1.f, -1.f, -1.f }, { 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, -1.f },
		// Bottom face
		{ -1.f, -1.f, -1.f }, { 1.f, -1.f, -1.f }, { 1.f, -1.f, 1.f }, { -1.f, -1.f, 1.f },
		// Top Face
		{ -1.f, 1.f, -1.f }, { 1.f, 0.f, -1.f }, { 1.f, 1.f, 1.f }, { -1.f, 1.f, 1.f }
	};

	Vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0, // Back
		4, 5, 6, 6, 7, 4, // Front
		8, 9, 10, 10, 11, 8, // Left
		12, 13, 14, 14, 15, 12, // Right
		16, 17, 18, 18, 19, 16, // Bottom
		20, 21, 22, 22, 23, 20 // Top
	};

	this->m_cubeVBO = this->CreateVertexBuffer(vertices);
	this->m_cubeIBO = this->CreateIndexBuffer(indices);

	spdlog::debug("VulkanRenderer::CreateCubeMesh: IBL Cube mesh created");
}

/*
	Create a frame buffer per each G-Buffer
*/
void VulkanRenderer::CreateGBufferFrameBuffer() {
	VkImageView attachments[] = {
		this->m_colorBuffView,
		this->m_normalBuffView,
		this->m_ormBuffView,
		this->m_emissiveBuffView,
		this->m_positionBuffView,
		this->m_depthImageView,
		this->m_colorResolveBuffView,
		this->m_normalResolveBuffView,
		this->m_ormResolveBuffView,
		this->m_emissiveResolveBuffView,
		this->m_positionResolveBuffView,
		this->m_depthResolveImageView
	};

	/* Our frame buffer create info */
	VkFramebufferCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.renderPass = this->m_geometryRenderPass;
	createInfo.width = this->m_scExtent.width;
	createInfo.height = this->m_scExtent.height;
	createInfo.layers = 1;
	createInfo.attachmentCount = 12;
	createInfo.pAttachments = attachments;

	if (vkCreateFramebuffer(this->m_device, &createInfo, nullptr, &this->m_gbufferFramebuffer) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateGBufferFrameBuffer: Failed creating G-Buffer frame buffer");
		throw std::runtime_error("VulkanRenderer::CreateGBufferFrameBuffer: Failed creating G-Buffer frame buffer");
		return;
	}
	
	spdlog::debug("VulkanRenderer::CreateGBufferFrameBuffer: G-Buffer frame buffer created");
}

/*
	Create framebuffers for drawing our screen quad
*/
void VulkanRenderer::CreateLightingFrameBuffer() {
	this->m_scFrameBuffers.resize(this->m_imageViews.size());

	for (uint32_t i = 0; i < this->m_scFrameBuffers.size(); i++) {
		VkImageView attachments[] = {
			this->m_imageViews[i]
		};

		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = this->m_lightingRenderPass;
		createInfo.width = this->m_scExtent.width;
		createInfo.height = this->m_scExtent.height;
		createInfo.layers = 1;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;

		if (vkCreateFramebuffer(this->m_device, &createInfo, nullptr, &this->m_scFrameBuffers[i]) != VK_SUCCESS) {
			spdlog::error("VulkanRenderer::CreateLightingFrameBuffer: Failed creating lighting frame buffer");
			throw std::runtime_error("VulkanRenderer::CreateLightingFrameBuffer: Failed creating lighting frame buffer");
			return;
		}
	}

	spdlog::debug("VulkanRenderer::CreateLightingFrameBuffer: Lighting frame buffers created");
}

/* 
	Create framebuffers for drawing our skybox
*/
void VulkanRenderer::CreateSkyboxFrameBuffer() {
	this->m_skyboxFrameBuffers.resize(this->m_imageViews.size());

	for (uint32_t i = 0; i < this->m_skyboxFrameBuffers.size(); i++) {
		VkImageView attachments[] = {
			this->m_imageViews[i]
		};

		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = this->m_skyboxRenderPass;
		createInfo.width = this->m_scExtent.width;
		createInfo.height = this->m_scExtent.height;
		createInfo.layers = 1;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;

		if (vkCreateFramebuffer(this->m_device, &createInfo, nullptr, &this->m_skyboxFrameBuffers[i]) != VK_SUCCESS) {
			spdlog::error("VulkanRenderer::CreateSkynoxFrameBuffer: Failed creating skybox frame buffer");
			throw std::runtime_error("VulkanRenderer::CreateSkynoxFrameBuffer: Failed creating skybox frame buffer");
			return;
		}
	}

	spdlog::debug("VulkanRenderer::CreateSkyboxFrameBuffer: Skybox frame buffers created");
}

/* Create our descriptor set layout */
void VulkanRenderer::CreateDescriptorSetLayout() {
	/* World view projection binding */
	VkDescriptorSetLayoutBinding wvpBinding = { };
	wvpBinding.binding = 0;
	wvpBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	wvpBinding.descriptorCount = 1;
	wvpBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	wvpBinding.pImmutableSamplers = nullptr;
	
	/* Descriptor set layout create info */
	VkDescriptorSetLayoutCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = &wvpBinding;
	createInfo.bindingCount = 1;
	
	/* Create our descriptor set layout */
	if (vkCreateDescriptorSetLayout(this->m_device, &createInfo, nullptr, &this->m_wvpDescriptorSetLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateDescriptorSetLayout: Failed creating WVP descriptor set layout");
		throw std::runtime_error("VulkanRenderer::CreateDescriptorSetLayout: Failed creating descriptor set layout");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateDescriptorSetLayout: WVP Descriptor set layout created");

	/* Texture sampler binding */
	VkDescriptorSetLayoutBinding samplerBinding = { };
	samplerBinding.binding = 0; // In this layout, binding 0 (independent from the other layout)
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = this->m_nMaxTextures;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	/* 
		Bindless descriptor indexing flags
		
		Specification: http://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkDescriptorBindingFlags.html
		Flag bits specification: https://registry.khronos.org/VulkanSC/specs/1.0-extensions/man/html/VkDescriptorBindingFlagBits.html
	*/

	VkDescriptorBindingFlags bindingFlags =
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

	/* Descriptor set layout binding flags create info */
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = { };
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsInfo.pBindingFlags = &bindingFlags;
	bindingFlagsInfo.bindingCount = 1;

	/* Descriptor set layout create info */
	VkDescriptorSetLayoutCreateInfo textureCreateInfo = { };
	textureCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureCreateInfo.pBindings = &samplerBinding;
	textureCreateInfo.bindingCount = 1;
	textureCreateInfo.pNext = &bindingFlagsInfo;
	textureCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

	/* Create our texture descriptor set layout */
	if (vkCreateDescriptorSetLayout(this->m_device, &textureCreateInfo, nullptr, &this->m_textureDescriptorSetLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateDescriptorSetLayout: Failed creating WVP descriptor set layout");
		throw std::runtime_error("VulkanRenderer::CreateDescriptorSetLayout: Failed creating descriptor set layout");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateDescriptorSetLayout: Texture Descriptor set layout created (bindless)");
}

/*
	Creates our descriptor set layout for our lighting pass
*/
void VulkanRenderer::CreateLightingDescriptorSetLayout() {
	/* Texture sampler binding */
	VkDescriptorSetLayoutBinding samplerBinding = { };
	samplerBinding.binding = 0;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.descriptorCount = 5;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[] = { samplerBinding };

	/* Descriptor set layout create info */
	VkDescriptorSetLayoutCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = bindings;
	createInfo.bindingCount = 1;
	
	if (vkCreateDescriptorSetLayout(this->m_device, &createInfo, nullptr, &this->m_lightingDescriptorSetLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateLightingDescriptorSetLayout: Failed creating lighting descriptor set layout");
		throw std::runtime_error("VulkanRenderer::CreateLightingDescriptorSetLayout: Failed creating lighting descriptor set layout");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateLightingDescriptorSetLayout: Lighting descriptor set layout created");
}

/*
	Creates our descriptor set layout for our skybox pass
*/
void VulkanRenderer::CreateSkyboxDescriptorSetLayout() {
	/* Cubemap sampler binding */
	VkDescriptorSetLayoutBinding cubemapBinding = { };
	cubemapBinding.binding = 0;
	cubemapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	cubemapBinding.descriptorCount = 1;
	cubemapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	cubemapBinding.pImmutableSamplers = nullptr;

	/* Depth sampler binding */
	VkDescriptorSetLayoutBinding depthBinding = { };
	depthBinding.binding = 1;
	depthBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	depthBinding.descriptorCount = 1;
	depthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	depthBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding bindings[] = { cubemapBinding, depthBinding };

	/* Descriptor set layout create info */
	VkDescriptorSetLayoutCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pBindings = bindings;
	createInfo.bindingCount = 2;

	if (vkCreateDescriptorSetLayout(this->m_device, &createInfo, nullptr, &this->m_skyboxDescriptorSetLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateSkyboxDescriptorSetLayout: Failed creating skybox descriptor set layout");
		throw std::runtime_error("VulkanRenderer::CreateSkyboxDescriptorSetLayout: Failed creating skybox descriptor set layout");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateSkyboxDescriptorSetLayout: Skybox descriptor set layout created");
}

/* Create our descriptor pool */
void VulkanRenderer::CreateDescriptorPool() {
	/* Define our uniform descriptor pool size */
	VkDescriptorPoolSize wvpPoolSize = { };
	wvpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	wvpPoolSize.descriptorCount = this->m_nImageCount;


	/* Descriptor pool create info */
	VkDescriptorPoolCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &wvpPoolSize;
	createInfo.maxSets = this->m_nImageCount;

	/* Create our descriptor pool */
	if (vkCreateDescriptorPool(this->m_device, &createInfo, nullptr, &this->m_wvpDescriptorPool) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateDescriptorPool: Failed creating WVP descriptor pool");
		throw std::runtime_error("VulkanRenderer::CreateDescriptorPool: Failed creating descriptor pool");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateDescriptorPool: WVP descriptor pool created");

	/* Texture pool size */
	VkDescriptorPoolSize texturePoolSize = { };
	texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texturePoolSize.descriptorCount = this->m_nMaxTextures;

	VkDescriptorPoolCreateInfo textureCreateInfo = { };
	textureCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	textureCreateInfo.poolSizeCount = 1;
	textureCreateInfo.pPoolSizes = &texturePoolSize;
	textureCreateInfo.maxSets = 1; // Only one global set
	textureCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	if (vkCreateDescriptorPool(this->m_device, &textureCreateInfo, nullptr, &this->m_textureDescriptorPool) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateDescriptorPool: Failed creating texture descriptor pool");
		throw std::runtime_error("VulkanRenderer::CreateDescriptorPool: Failed creating descriptor pool");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateDescriptorPool: Texture descriptor pool created");

}

/*
	Create a descriptor pool for our lighting pass 
*/
void VulkanRenderer::CreateLightingDescriptorPool() {
	/* Define our descriptor pool size */
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 5 * this->m_nImageCount; // 5 samplers (base color, normal, ORM, emissive, position) per frame

	/* Descriptor pool create info */
	VkDescriptorPoolCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &poolSize;
	createInfo.maxSets = this->m_nImageCount; // One per frame

	/* Create our descriptor pool */
	if (vkCreateDescriptorPool(this->m_device, &createInfo, nullptr, &this->m_lightingDescriptorPool) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateLightingDescriptorPool: Failed creating lighting descriptor pool");
		throw std::runtime_error("VulkanRenderer::CreateLightingDescriptorPool: Failed creating lighting descriptor pool");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateLightingDescriptorPool: Lighting descriptor pool created");
}

/*
	Create a descriptor pool for our skybox pass
*/
void VulkanRenderer::CreateSkyboxDescriptorPool() {
	/* Define our descriptor pool size */
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = 2 * this->m_nImageCount;

	/* Descriptor pool create info */
	VkDescriptorPoolCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &poolSize;
	createInfo.maxSets = this->m_nImageCount;

	/* Create our descriptor pool */
	if (vkCreateDescriptorPool(this->m_device, &createInfo, nullptr, &this->m_skyboxDescriptorPool) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateSkyboxDescriptorPool: Failed creating skybox descriptor pool");
		throw std::runtime_error("VulkanRenderer::CreateSkyboxDescriptorPool: Failed creating skybox descriptor pool");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateSkyboxDescriptorPool: Skybox descriptor pool created");
}

/* 
	Allocate our descriptor sets 
*/
void VulkanRenderer::AllocateDescriptorSets() {
	/* 
		Initialize vector with m_nImageCount size and copy in each index m_descriptorSetLayout 
		We do this because we are going to have a descriptor set per frame in flight
	*/
	Vector<VkDescriptorSetLayout> wvpLayouts(this->m_nImageCount, this->m_wvpDescriptorSetLayout);

	/* 
		Allocate info

		VkDescriptorSetAllocateInfo:	
			- descriptorPool: Reference to our descriptor pool
			- descriptorSetCount: Our frames in flight count
			- pSetLayouts: Our decriptor set layouts
	*/
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->m_wvpDescriptorPool;
 	allocInfo.pSetLayouts = wvpLayouts.data();
	allocInfo.descriptorSetCount = this->m_nImageCount;

	/* Allocate our descriptor sets */
	this->m_descriptorSets.resize(this->m_nImageCount);
	if (vkAllocateDescriptorSets(this->m_device, &allocInfo, this->m_descriptorSets.data()) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::AllocateDescriptorSets: Failed allocating WVP descriptor sets");
		throw std::runtime_error("VulkanRenderer::AllocateDescriptorSets: Failed allocating descriptor sets");
		return;
	}

	spdlog::debug("VulkanRenderer::AllocateDescriptorSets: WVP Descriptor sets allocated");

	/* Allocate global texture descriptor set */
	VkDescriptorSetAllocateInfo textureAllocInfo = { };
	textureAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	textureAllocInfo.descriptorPool = this->m_textureDescriptorPool;
	textureAllocInfo.pSetLayouts = &this->m_textureDescriptorSetLayout;
	textureAllocInfo.descriptorSetCount = 1;

	if (vkAllocateDescriptorSets(this->m_device, &textureAllocInfo, &this->m_globalTextureDescriptorSet) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::AllocateDescriptorSets: Failed allocating global texture descriptor set");
		throw std::runtime_error("VulkanRenderer::AllocateDescriptorSets: Failed allocating descriptor sets");
		return;
	}

	spdlog::debug("VulkanRenderer::AllocateDescriptorSets: Global texture descriptor set allocated");
}

/* Allocate descriptor sets for our lighting pass */
void VulkanRenderer::AllocateLightingDescriptorSets() {
	Vector<VkDescriptorSetLayout> layouts(this->m_nImageCount, this->m_lightingDescriptorSetLayout);

	/* Descriptor set allocate info */
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->m_lightingDescriptorPool;
	allocInfo.descriptorSetCount = this->m_nImageCount;
	allocInfo.pSetLayouts = layouts.data();

	this->m_lightingDescriptorSets.resize(this->m_nImageCount);
	if (vkAllocateDescriptorSets(this->m_device, &allocInfo, this->m_lightingDescriptorSets.data()) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::AllocateLightingDescriptorSets: Failed allocating lighting descriptor sets");
		throw std::runtime_error("VulkanRenderer::AllocateLightingDescriptorSets: Failed allocating lighting descriptor sets");
		return;
	}

	spdlog::debug("VulkanRenderer::AllocateLightingDescriptorSets: Lighting descriptor sets allocated");
}

/* Allocate descriptor sets for our skybox pass */
void VulkanRenderer::AllocateSkyboxDescriptorSets() {
	Vector<VkDescriptorSetLayout> layouts(this->m_nImageCount, this->m_skyboxDescriptorSetLayout);

	/* Descriptor set allocate info */
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = this->m_skyboxDescriptorPool;
	allocInfo.descriptorSetCount = this->m_nImageCount;
	allocInfo.pSetLayouts = layouts.data();

	this->m_skyboxDescriptorSets.resize(this->m_nImageCount);
	if (vkAllocateDescriptorSets(this->m_device, &allocInfo, this->m_skyboxDescriptorSets.data()) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::AllocateSkyboxDescriptorSets: Failed allocating skybox descriptor sets");
		throw std::runtime_error("VulkanRenderer::AllocateSkyboxDescriptorSets: Failed allocating skybox descriptor sets");
		return;
	}

	spdlog::debug("VulkanRenderer::AllocateSkyboxDescriptorSets: Skybox descriptor sets allocated");
}

/* Writes our descriptor sets */
void VulkanRenderer::WriteDescriptorSets() {
	/* Get our World View Projection Ring Buffer */
	VulkanRingBuffer* ringBuffer = dynamic_cast<VulkanRingBuffer*>(this->m_wvpBuff);
	if (ringBuffer == nullptr) {
		spdlog::error("VulkanRenderer::WriteDescriptorSets: Selected ring buffer is not a vulkan ring buffer");
		throw std::runtime_error("VulkanRenderer::WriteDescriptorSets: Selected ring buffer is not a vulkan ring buffer");
		return;
	}

	/* Write each of our WVP descriptor sets */
	for (uint32_t i = 0; i < this->m_nImageCount; i++) {
		/* WVP Ring Buffer info */
		VkDescriptorBufferInfo buffInfo = { };
		buffInfo.buffer = ringBuffer->GetBuffer(); // Our VkBuffer
		buffInfo.offset = 0; // 0 because we are going to use dynamic offsets
		buffInfo.range = sizeof(WVP);

		/* Write to descriptor set */
		VkWriteDescriptorSet descriptorWrite = { };
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = this->m_descriptorSets[i];
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &buffInfo;

		vkUpdateDescriptorSets(this->m_device, 1, &descriptorWrite, 0, nullptr);
	}

	spdlog::debug("VulkanRenderer::WriteDescriptorSets: WVP descriptor sets written");
}

void VulkanRenderer::WriteLightDescriptorSets() {
	VkDescriptorImageInfo colorInfo = { };
	colorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	colorInfo.imageView = this->m_colorResolveBuffView;
	colorInfo.sampler = this->m_baseColorSampler;

	VkDescriptorImageInfo normalInfo = { };
	normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	normalInfo.imageView = this->m_normalResolveBuffView;
	normalInfo.sampler = this->m_normalSampler;

	VkDescriptorImageInfo ormInfo = { };
	ormInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	ormInfo.imageView = this->m_ormResolveBuffView;
	ormInfo.sampler = this->m_ormSampler;

	VkDescriptorImageInfo emissiveInfo = { };
	emissiveInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emissiveInfo.imageView = this->m_emissiveResolveBuffView;
	emissiveInfo.sampler = this->m_emissiveSampler;

	VkDescriptorImageInfo positionInfo = { };
	positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	positionInfo.imageView = this->m_positionResolveBuffView;
	positionInfo.sampler = this->m_positionSampler;

	for (uint32_t i = 0; i < this->m_lightingDescriptorSets.size(); i++) {
		VkWriteDescriptorSet descriptorWrites[5] = { };

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = this->m_lightingDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].pImageInfo = &colorInfo;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = this->m_lightingDescriptorSets[i];
		descriptorWrites[1].dstBinding = 0;
		descriptorWrites[1].dstArrayElement = 1;
		descriptorWrites[1].pImageInfo = &normalInfo;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
						 
		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = this->m_lightingDescriptorSets[i];
		descriptorWrites[2].dstBinding = 0;
		descriptorWrites[2].dstArrayElement = 2;
		descriptorWrites[2].pImageInfo = &ormInfo;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = this->m_lightingDescriptorSets[i];
		descriptorWrites[3].dstBinding = 0;
		descriptorWrites[3].dstArrayElement = 3;
		descriptorWrites[3].pImageInfo = &emissiveInfo;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;

		descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[4].dstSet = this->m_lightingDescriptorSets[i];
		descriptorWrites[4].dstBinding = 0;
		descriptorWrites[4].dstArrayElement = 4;
		descriptorWrites[4].pImageInfo = &positionInfo;
		descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[4].descriptorCount = 1;

		vkUpdateDescriptorSets(this->m_device, 5, descriptorWrites, 0, nullptr);
	}

	spdlog::debug("VulkanRenderer::WriteLightDescriptorSets: Lighting descriptor sets written");
}

/*
	Write our skybox descriptor sets
	
	TODO: Make this function able to change skyboxes dynamically.
*/
void VulkanRenderer::WriteSkyboxDescriptorSets() {
	/* 
		Get our skybox VulkanTexture
		TODO: Select skybox from current scene.
	*/
	VulkanTexture* vkSkybox = dynamic_cast<VulkanTexture*>(this->m_skybox);
	VkImageView skyboxView = vkSkybox->GetImageView();
	VkSampler skyboxSampler = vkSkybox->GetSampler();

	/* Descriptor image info */
	VkDescriptorImageInfo skyboxInfo = { };
	skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	skyboxInfo.imageView = skyboxView;
	skyboxInfo.sampler = skyboxSampler;

	VkDescriptorImageInfo depthInfo = { };
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	depthInfo.imageView = this->m_depthResolveImageView;
	depthInfo.sampler = this->m_depthSampler;
	
	for (uint32_t i = 0; i < this->m_skyboxDescriptorSets.size(); i++) {
		VkWriteDescriptorSet descriptorWrites[2] = { };

		/* Cubemap */
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = this->m_skyboxDescriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].pImageInfo = &skyboxInfo;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;

		/* Depth buffer */
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = this->m_skyboxDescriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].pImageInfo = &depthInfo;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;

		vkUpdateDescriptorSets(this->m_device, 2, descriptorWrites, 0, nullptr);
	}

	spdlog::debug("VulkanRenderer::WriteSkyboxDescriptorSets: Skybox descriptor sets written");
}

void VulkanRenderer::CreateCommandBuffer() {
	this->m_commandBuffers.resize(this->m_nImageCount);

	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = this->m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandBufferCount = this->m_nImageCount;

	if (vkAllocateCommandBuffers(this->m_device, &allocInfo, this->m_commandBuffers.data()) != VK_SUCCESS) {
		spdlog::error("CreateCommandBuffer: Error allocating command buffer");
		throw std::runtime_error("CreateCommandBuffer: Error allocating command buffer");
		return;
	}

	spdlog::debug("CreateCommandBuffer: Command buffer allocated");
}

/* 
	Create a graphics pipeline.
	Parameters:
		- vertPath & pixelPath: Path to Vertex and Pixel shaders
		- renderPass: Render pass
		- setLayouts: Pointer to our Descriptor set layout list
		- nSetLayerCount: The ammount of descriptor set layouts
		- vertexInfo: The input state of our vertex shader
		- rasterizer: Our rasterizer info
		- multisampling: Our multisampler state info
		- depthStencil: Our depth stencil state info
		- colorBlend: Our color blend information
		- pLayout: A pointer to the 

	Return value: The created pipeline.
*/
VkPipeline VulkanRenderer::CreateGraphicsPipeline(
	const String& vertPath,
	const String& pixelPath,
	VkRenderPass renderPass,
	VkDescriptorSetLayout* setLayouts,
	uint32_t nSetLayoutCount,
	VkPipelineVertexInputStateCreateInfo vertexInfo,
	VkPipelineRasterizationStateCreateInfo rasterizer,
	VkPipelineMultisampleStateCreateInfo multisampling,
	VkPipelineDepthStencilStateCreateInfo depthStencil,
	VkPipelineColorBlendStateCreateInfo colorBlend,
	VkPipelineLayout* pLayout,
	VkPushConstantRange* pPushConstantRanges,
	uint32_t nPushConstantCount
) {
	/* Shader compiling */
	Vector<uint32_t> vertexShader = this->CompileShader(this->ReadShader(vertPath), vertPath, shaderc_vertex_shader);
	Vector<uint32_t> fragmentShader = this->CompileShader(this->ReadShader(pixelPath), pixelPath, shaderc_fragment_shader);

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

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo layoutInfo = { };
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pSetLayouts = setLayouts;
	layoutInfo.setLayoutCount = nSetLayoutCount;
	layoutInfo.pPushConstantRanges = pPushConstantRanges,
	layoutInfo.pushConstantRangeCount = nPushConstantCount;

	/* Create pipeline layout */
	if (vkCreatePipelineLayout(this->m_device, &layoutInfo, nullptr, pLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateGraphicsPipeline: Failed creating pipeline layout");
		throw std::runtime_error("VulkanRenderer::CreateGraphicsPipeline: Failed creating pipeline layout");
		return nullptr;
	}

	/* Viewport / scissor */
	VkViewport viewport = {  };
	viewport.width = static_cast<float>(this->m_scExtent.width);
	viewport.height = -static_cast<float>(this->m_scExtent.height); // Flip our viewport for +Y up
	viewport.x = 0.f;
	viewport.y = static_cast<float>(this->m_scExtent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	this->m_viewport = viewport;

	VkRect2D scissor = { };
	scissor.extent = this->m_scExtent;
	this->m_scissor = scissor;

	/* Pipeline viewport state create info */
	VkPipelineViewportStateCreateInfo viewportInfo = { };
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &this->m_viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &this->m_scissor;

	/* Dynamic states */
	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = { };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStates;
	dynamicState.dynamicStateCount = 2;

	/* Assembly creation info */
	VkPipelineInputAssemblyStateCreateInfo inputInfo = { };
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* Pipeline */
	VkGraphicsPipelineCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = shaderStages;
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &inputInfo;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pRasterizationState = &rasterizer;
	createInfo.pMultisampleState = &multisampling;
	createInfo.pDepthStencilState = &depthStencil;
	createInfo.pColorBlendState = &colorBlend;
	createInfo.pDynamicState = &dynamicState;
	createInfo.layout = *pLayout;
	createInfo.renderPass = renderPass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(this->m_device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateGraphicsPipeline: Failed creating graphics pipeline");
		throw std::runtime_error("VulkanRenderer::CreateGraphicsPipeline: Failed creating graphics pipeline");
		return nullptr;
	}

	/* Destroy shader modules */
	vkDestroyShaderModule(this->m_device, vertexModule, nullptr);
	vkDestroyShaderModule(this->m_device, fragmentModule, nullptr);

	return pipeline;
}

void VulkanRenderer::CreateGBufferPipeline() {
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
	attribs[0].offset = offsetof(Vertex, position);

	attribs[1].binding = 0;
	attribs[1].location = 1;
	attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[1].offset = offsetof(Vertex, normal);

	attribs[2].binding = 0; 
	attribs[2].location = 2;
	attribs[2].format = VK_FORMAT_R32G32_SFLOAT;
	attribs[2].offset = offsetof(Vertex, texCoord);

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

	/* Our rasterizer state creation info */
	VkPipelineRasterizationStateCreateInfo rasterizer = { };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;

	/* Our multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = { };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = this->m_multisampleCount;
	multisampling.sampleShadingEnable = VK_TRUE; // Improves the quality 
	multisampling.minSampleShading = .2f; // Minimum 20% shading per frame

	/* Color blend (we have 5: albedo, normal, ORM, emissive, position) */
	VkPipelineColorBlendAttachmentState colorBlends[5] = { };

	/* For each attachment we disable blending and enable the write mask in all RGBA channels */
	for (uint32_t i = 0; i < 5; i++) {
		colorBlends[i].blendEnable = VK_FALSE;
		colorBlends[i].colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo blendState = { };
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.attachmentCount = 5;
	blendState.pAttachments = colorBlends;
	blendState.logicOpEnable = VK_FALSE;

	/* Pipeline depth stencil state create info */
	VkPipelineDepthStencilStateCreateInfo depthStencil = { };
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkDescriptorSetLayout setLayouts[] = {
		this->m_wvpDescriptorSetLayout,
		this->m_textureDescriptorSetLayout
	};

	VkPushConstantRange pushRange = { };
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(PushConstant);
	
	/* Create G-Buffer pipeline with CreateGraphicsPipeline method */
	this->m_gbuffPipeline = this->CreateGraphicsPipeline(
		"GBufferPass.vert", "GBufferPass.frag",
		this->m_geometryRenderPass, 
		setLayouts, 2,
		vertexInfo, 
		rasterizer,
		multisampling,
		depthStencil, 
		blendState, 
		&this->m_gbuffPipelineLayout,
		&pushRange,
		1
	);
	
	spdlog::debug("CreateGBufferPipeline: G-Buffer pipeline created");
}

void VulkanRenderer::CreateLightingPipeline() {
	/* Input binding */
	VkVertexInputBindingDescription bindingDesc = { };
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(ScreenQuadVertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attribs[2] = {};
	attribs[0].binding = 0;
	attribs[0].location = 0;
	attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[0].offset = offsetof(ScreenQuadVertex, position);

	attribs[1].binding = 0;
	attribs[1].location = 1;
	attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
	attribs[1].offset = offsetof(ScreenQuadVertex, texCoord);

	/* Pipeline vertex input state create info */
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexAttributeDescriptions = attribs;
	vertexInfo.vertexAttributeDescriptionCount = 2;
	
	/* Input assembly create info */
	VkPipelineInputAssemblyStateCreateInfo inputInfo = { };
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* Rasterizer */
	VkPipelineRasterizationStateCreateInfo rasterizer = { };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = { };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;

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
	blendState.pAttachments = &colorBlend;
	blendState.attachmentCount = 1;
	blendState.logicOpEnable = VK_FALSE;

	/* Pipeline depth stencil state create info */
	VkPipelineDepthStencilStateCreateInfo depthStencil = { };
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkDescriptorSetLayout setLayouts[] = {
		this->m_lightingDescriptorSetLayout
	};

	VkPushConstantRange pushRange = { };
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(LightPushConstant);

	this->m_lightingPipeline = this->CreateGraphicsPipeline(
		"LightingPass.vert",
		"LightingPass.frag",
		this->m_lightingRenderPass,
		setLayouts, 1,
		vertexInfo,
		rasterizer,
		multisampling,
		depthStencil,
		blendState,
		&this->m_lightingPipelineLayout,
		&pushRange,
		1
	);
}

void VulkanRenderer::CreateSkyboxPipeline() {
	/* Input binding */
	VkVertexInputBindingDescription bindingDesc = { };
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(ScreenQuadVertex);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attribs[2] = {};
	attribs[0].binding = 0;
	attribs[0].location = 0;
	attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[0].offset = offsetof(ScreenQuadVertex, position);

	attribs[1].binding = 0;
	attribs[1].location = 1;
	attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
	attribs[1].offset = offsetof(ScreenQuadVertex, texCoord);

	/* Pipeline vertex input state create info */
	VkPipelineVertexInputStateCreateInfo vertexInfo = { };
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexAttributeDescriptions = attribs;
	vertexInfo.vertexAttributeDescriptionCount = 2;

	/* Input assembly create info */
	VkPipelineInputAssemblyStateCreateInfo inputInfo = { };
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* Rasterizer */
	VkPipelineRasterizationStateCreateInfo rasterizer = { };
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = { };
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;

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
	blendState.pAttachments = &colorBlend;
	blendState.attachmentCount = 1;
	blendState.logicOpEnable = VK_FALSE;

	/* Pipeline depth stencil state create info */
	VkPipelineDepthStencilStateCreateInfo depthStencil = { };
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkDescriptorSetLayout setLayouts[] = {
		this->m_skyboxDescriptorSetLayout
	};

	/* Pipeline layout create info */
	VkPipelineLayoutCreateInfo layoutInfo = { };
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.pSetLayouts = setLayouts;
	layoutInfo.setLayoutCount = 1;

	if (vkCreatePipelineLayout(this->m_device, &layoutInfo, nullptr, &this->m_skyboxPipelineLayout) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateSkyboxPipeline: Failed creating skybox pipeline");
		throw std::runtime_error("VulkanRenderer::CreateSkyboxPipeline: Failed creating skybox pipeline");
		return;
	}

	spdlog::debug("VulkanRenderer::CreateSkyboxPipeline: Skybox pipeline layout created");

	VkPushConstantRange pushRange = { };
	pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(SkyboxPushConstant);

	this->m_skyboxPipeline = this->CreateGraphicsPipeline(
		"SkyboxPass.vert",
		"SkyboxPass.frag",
		this->m_skyboxRenderPass,
		setLayouts, 1,
		vertexInfo,
		rasterizer,
		multisampling,
		depthStencil,
		blendState,
		&this->m_skyboxPipelineLayout,
		&pushRange,
		1
	);
}

/* Creation of our sync objects */
void VulkanRenderer::CreateSyncObjects() {
	this->m_imageAvailableSemaphores.resize(this->m_nImageCount);
	this->m_renderFinishedSemaphores.resize(this->m_nImageCount);
	this->m_inFlightFences.resize(this->m_nImageCount);

	VkSemaphoreCreateInfo semaphoreInfo = { };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = { };
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(uint32_t i = 0; i < this->m_nImageCount; i++) {
		if (vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_inFlightFences[i]) != VK_SUCCESS) {
			spdlog::error("CreateSyncObjects: Failed to create sync objects");
			throw std::runtime_error("CreateSyncObjects: Failed to create sync objects");
			return;
		}
	}


	spdlog::debug("CreateSyncObjects: Sync objects created");
}

void VulkanRenderer::RecordCommandBuffer(uint32_t nImageIndex) {
	/* Begin command buffer */
	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkCommandBuffer commandBuffer = this->m_commandBuffers[this->m_nCurrentFrameIndex];

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	/* Begin render pass */

	/* G-Buffer pass */
	/* G-Buffer clear values */
	VkClearValue albedoClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue normalClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue ormClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue emissiveClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue positionClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue albedoResolveClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue normalResolveClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue ormResolveClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue emissiveResolveClear = { { { 0.f, 0.f, 0.f, 1.f } } };
	VkClearValue positionResolveClear = { { { 0.f, 0.f, 0.f, 1.f } } };

	/* Depth clear value */
	VkClearValue depthClear = { };
	depthClear.depthStencil = { 1.f, 0 };

	VkClearValue depthResolveClear = { };
	depthResolveClear.depthStencil = { 1.f, 0 };

	VkClearValue gbuffClears[] = {
		albedoClear,
		normalClear,
		ormClear,
		emissiveClear,
		positionClear,
		depthClear,
		albedoResolveClear,
		normalResolveClear,
		ormResolveClear,
		emissiveResolveClear,
		positionResolveClear,
		depthResolveClear
	};

	VkRenderPassBeginInfo geometryPassInfo = { };
	geometryPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	geometryPassInfo.clearValueCount = 12;
	geometryPassInfo.pClearValues = gbuffClears;
	geometryPassInfo.renderArea.extent = this->m_scExtent;
	geometryPassInfo.renderArea.offset = { 0, 0 };
	geometryPassInfo.renderPass = this->m_geometryRenderPass;
	geometryPassInfo.framebuffer = this->m_gbufferFramebuffer;

	vkCmdBeginRenderPass(commandBuffer, &geometryPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	/* Bind G-Buffer pipeline */
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_gbuffPipeline);
	vkCmdSetViewport(commandBuffer, 0, 1, &this->m_viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &this->m_scissor);

	/* Get current Scene */
	Scene* currentScene = this->m_sceneMgr->GetCurrentScene();
	Camera* currentCamera = currentScene->GetCurrentCamera();
	Transform cameraTransform = currentCamera->transform;
	std::map<String, GameObject*> objects = currentScene->GetObjects();

	glm::mat4 cameraMatrix = glm::mat4(1.f);
	cameraMatrix = glm::rotate(cameraMatrix, glm::radians(cameraTransform.rotation.x), glm::vec3(1.f, 0.f, 0.f));
	cameraMatrix = glm::rotate(cameraMatrix, glm::radians(cameraTransform.rotation.y), glm::vec3(0.f, 1.f, 0.f));
	cameraMatrix = glm::translate(cameraMatrix, {
		-cameraTransform.location.x,
		-cameraTransform.location.y,
		cameraTransform.location.z
	});

	glm::mat4 projMatrix = glm::perspectiveFovRH(glm::radians(70.f), static_cast<float>(this->m_scExtent.width), static_cast<float>(this->m_scExtent.height), .001f, 300.f);

	this->m_wvp.View = cameraMatrix;
	this->m_wvp.Projection = projMatrix;

	/* TODO: Change the WVP allocation to 'per-object' data */
	uint32_t nDynamicOffset = 0;
	this->m_wvpBuff->Reset(nImageIndex); // Reset our World View Projection ring buffer
	void* pMap = this->m_wvpBuff->Allocate(sizeof(this->m_wvp), nDynamicOffset); // Allocate a new uniform WVP
	memcpy(pMap, &this->m_wvp, sizeof(this->m_wvp)); // Copy data to buffer

	/* Bind descriptor sets */

	/* 
		World view projection:
		Will bind at set 0
		We'll use the corresponding descriptor set to the current image
		We'll use dynamic offsets (given by our ring buffer)
	*/
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->m_gbuffPipelineLayout,
		0,
		1,
		&this->m_descriptorSets[nImageIndex],
		1,
		&nDynamicOffset
	);

	/*
		Global texture descriptor set binding (bindless textures)
		We'll bind it at set 1
		We'll use 0 dynamic offsets
	*/
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->m_gbuffPipelineLayout,
		1,
		1,
		&this->m_globalTextureDescriptorSet,
		0,
		nullptr
	);

	/* Per object loop */
	for (std::pair<String, GameObject*> obj : objects) {
		/* Get only the GameObject pointer, we actually don't care the object name */
		GameObject* pObj = obj.second;

		/* Get all the components from de object */
		std::map<String, Component*> components = pObj->GetComponents();
		
		/*
			Find the mesh component(s) and draw it

			TODO: Cache all the mesh components and reference to 
			their corresponding object and draw them.
		*/
		if (components.count("MeshComponent") <= 0) {
			continue; // Oh, you have no mesh components? NEXT.
		}

		Component* meshComponent = components["MeshComponent"];
		Mesh* mesh = dynamic_cast<Mesh*>(meshComponent);

		if (!mesh) {
			spdlog::error("VulkanRenderer::RecordCommandBuffer: Tried to cast type Component to MeshComponent and failed. GameObject: {0}", obj.first);
			continue;
		}

		std::map<uint32_t, GPUBuffer*> vertices = mesh->GetVBOs();
		std::map<uint32_t, uint32_t> textureIndices = mesh->GetTextureIndices();
		std::map<uint32_t, uint32_t> ormIndices = mesh->GetORMIndices();
		std::map<uint32_t, uint32_t> emissiveIndices = mesh->GetEmissiveIndices();

		uint32_t i = 0;
		for (std::pair<uint32_t, GPUBuffer*> vertex : vertices) {
			uint32_t nTextureIndex = textureIndices[vertex.first];
			uint32_t nOrmIndex = ormIndices[vertex.first];
			uint32_t nEmissiveIndex = INVALID_INDEX;

			if (std::map<uint32_t, uint32_t>::iterator it = emissiveIndices.find(vertex.first); it != emissiveIndices.end()) {
				nEmissiveIndex = it->second;
			}

			PushConstant pushConstant = { nTextureIndex, nOrmIndex, nEmissiveIndex };

			vkCmdPushConstants(
				commandBuffer, 
				this->m_gbuffPipelineLayout, 
				VK_SHADER_STAGE_FRAGMENT_BIT, 
				0, 
				sizeof(PushConstant), 
				&pushConstant
			);
			
			if (mesh->HasIndices()) {
				std::map<uint32_t, GPUBuffer*> indices =  mesh->GetIBOs();
				GPUBuffer* index = indices[i];
				this->DrawIndexBuffer(vertex.second, index);
			}
			else {
				this->DrawVertexBuffer(vertex.second);
			}

			i++;
		}
	}

	vkCmdEndRenderPass(commandBuffer);

	/* Lighting pass */

	VkClearValue sqClear = { { { 0.f, 0.f, 0.f, 1.f } } };

	VkRenderPassBeginInfo lightingPassInfo = { };
	lightingPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	lightingPassInfo.clearValueCount = 1;
	lightingPassInfo.pClearValues = &sqClear;
	lightingPassInfo.renderArea.extent = this->m_scExtent;
	lightingPassInfo.renderArea.offset = { 0, 0 };
	lightingPassInfo.renderPass = this->m_lightingRenderPass;
	lightingPassInfo.framebuffer = this->m_scFrameBuffers[nImageIndex];

	LightPushConstant lightPushConstant = { { cameraTransform.location.x, cameraTransform.location.y, cameraTransform.location.z } };

	vkCmdBeginRenderPass(commandBuffer, &lightingPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->m_lightingPipelineLayout,
		0,
		1,
		&this->m_lightingDescriptorSets[nImageIndex],
		0,
		nullptr
	);

	vkCmdPushConstants(
		commandBuffer,
		this->m_lightingPipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(LightPushConstant),
		&lightPushConstant
	);
	
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_lightingPipeline);
	if(!this->DrawIndexBuffer(this->m_sqVBO, this->m_sqIBO)) {
		spdlog::error("VulkanRenderer::RecordCommandBuffer: Failed drawing ScreenQuad VBO/IBO");
		throw std::runtime_error("VulkanRenderer::RecordCommandBuffer: Failed drawing ScreenQuad VBO/IBO");
		return;
	}
	
	vkCmdEndRenderPass(commandBuffer);

	/* Skybox pass */
	//this->TransitionImageLayout(this->m_depthResolveImage, this->FindDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	VkRenderPassBeginInfo skyboxPassInfo = { };
	skyboxPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	skyboxPassInfo.clearValueCount = 0;
	skyboxPassInfo.pClearValues = nullptr;
	skyboxPassInfo.renderArea.extent = this->m_scExtent;
	skyboxPassInfo.renderArea.offset = { 0, 0 };
	skyboxPassInfo.renderPass = this->m_skyboxRenderPass;
	skyboxPassInfo.framebuffer = this->m_skyboxFrameBuffers[nImageIndex];

	SkyboxPushConstant skyboxPushConstant = { };
	skyboxPushConstant.cameraPosition = { cameraTransform.location.x, cameraTransform.location.y, cameraTransform.location.z };
	skyboxPushConstant.inverseView = glm::affineInverse(cameraMatrix);
	skyboxPushConstant.inverseProjection = glm::inverse(projMatrix);
	
	vkCmdBeginRenderPass(commandBuffer, &skyboxPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->m_skyboxPipelineLayout,
		0,
		1,
		&this->m_skyboxDescriptorSets[nImageIndex],
		0,
		nullptr
	);

	vkCmdPushConstants(
		commandBuffer,
		this->m_skyboxPipelineLayout,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(SkyboxPushConstant),
		&skyboxPushConstant
	);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_skyboxPipeline);
	if (!this->DrawIndexBuffer(this->m_sqVBO, this->m_sqIBO)) {
		spdlog::error("VulkanRenderer::RecordCommandBuffer: Failed drawing ScreenQuad VBO/IBO (skybox)");
		throw std::runtime_error("VulkanRenderer::RecordCommandBuffer: Failed drawing ScreenQuad VBO/IBO (skybox)");
		return;
	}

	vkCmdEndRenderPass(commandBuffer);

	vkEndCommandBuffer(commandBuffer);
}

/* 
	Choose for the best swap surface format

	If there is a format that is B8G8R8A8_SRGB and its color space is SRGB_NONLINEAR, select that
*/
VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const Vector<VkSurfaceFormatKHR>& formats) {
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
VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const Vector<VkPresentModeKHR>& presentModes) {
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
String VulkanRenderer::ReadShader(const String& sFile) {
	std::filesystem::path path = sFile;
	if(!path.is_absolute()) {
		String executableDir = GetExecutableDir();
		path = std::filesystem::path(executableDir) / sFile;
	}

	std::ifstream file(path);
	if (!file.is_open()) {
		spdlog::error("Couldn't open shader {0}", path.string());
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
Vector<uint32_t> VulkanRenderer::CompileShader(String shader, String filename, shaderc_shader_kind kind) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	options.SetForcedVersionProfile(450, shaderc_profile_none);
	options.SetTargetSpirv(shaderc_spirv_version_1_5);

	shaderc::CompilationResult preprocessedResult = compiler.PreprocessGlsl(shader, kind, filename.c_str(), options);
	if (preprocessedResult.GetCompilationStatus() != shaderc_compilation_status_success) {
		spdlog::error("Shader Preprocess Error: {}", preprocessedResult.GetErrorMessage());
		throw std::runtime_error("Shader preprocessor error");
	}

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shader, kind, filename.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		spdlog::error("CompileShader: {0}", result.GetErrorMessage());
		throw std::runtime_error("CompileShader: Error compiling shader");
		return Vector<uint32_t>();
	}

	spdlog::debug("CompileShader: Shader {0} compiled", filename);

	Vector<uint32_t> shaderCode(result.cbegin(), result.cend());
	return shaderCode;
}

/* Create a shader module from our shader code */
VkShaderModule VulkanRenderer::CreateShaderModule(Vector<uint32_t>& shaderCode) {
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
VkFormat VulkanRenderer::FindSupportedFormat(const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags) {
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

/* Create a buffer based on the buffer type */
GPUBuffer* VulkanRenderer::CreateBuffer(const void* pData, uint32_t nSize, EBufferType bufferType) {
	VkBufferUsageFlagBits usage = ToVkBufferUsage(bufferType);

	/* Buffer create info */
	VkBufferCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = nSize;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.usage = usage;

	/* Create buffer */
	VkBuffer buffer = nullptr;
	if (vkCreateBuffer(this->m_device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateBuffer: Failed creating buffer");
		throw std::runtime_error("VulkanRenderer::CreateBuffer: Failed creating buffer");
		return nullptr;
	}

	/* Get memory requirements */
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
	allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	/* Allocate memory */
	VkDeviceMemory memory = nullptr;
	if (vkAllocateMemory(this->m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateBuffer: Failed allocating buffer device memory");
		throw std::runtime_error("VulkanRenderer::CreateBuffer: Failed allocating buffer device memory");
		return nullptr;
	}

	/* Map our buffer device memory and copy our data */
	if (pData != nullptr) {
		void* pMapData = nullptr;
		vkMapMemory(this->m_device, memory, 0, VK_WHOLE_SIZE, 0, &pMapData);
		memcpy(pMapData, pData, nSize);
		vkUnmapMemory(this->m_device, memory);
	}

	/* Bind our buffer to our device memory */
	if (vkBindBufferMemory(this->m_device, buffer, memory, 0) != VK_SUCCESS) {
		spdlog::error("VulkanRenderer::CreateBuffer: Failed binding buffer memory");
		throw std::runtime_error("VulkanRenderer::CreateBuffer: Failed binding buffer memory");
		return nullptr;
	}

	VulkanBuffer* vkBuffer = new VulkanBuffer(this->m_device, this->m_physicalDevice, buffer, memory, nSize, bufferType);
	return vkBuffer;
}

/* Creation of a index buffer */
GPUBuffer* VulkanRenderer::CreateIndexBuffer(const Vector<uint16_t>& indices) {
	GPUBuffer* retBuff = this->CreateBuffer(static_cast<const void*>(indices.data()), indices.size() * sizeof(uint16_t), EBufferType::INDEX_BUFFER);
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

	VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	uint32_t nImageSize = this->CreateImage(
		nWidth, nHeight,
		vkFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		usageFlags,
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

/* Checks if texture is registered and returns the index */
uint32_t VulkanRenderer::GetTextureIndex(String& textureName) {
	if (this->m_textureIndices.count(textureName) <= 0) {
		spdlog::error("VulkanRenderer::GetTextureIndex: Texture {0} not found. Make sure it is registered", textureName);
		return 0;
	}

	return this->m_textureIndices[textureName];
}

/* 
	Registers a new texture on our texture indices 
	and loaded textures vector and writes to the 
	global texture descriptor set 
*/
uint32_t VulkanRenderer::RegisterTexture(const String& textureName, GPUTexture* pTexture) {
	VulkanTexture* texture = dynamic_cast<VulkanTexture*>(pTexture);
	if (texture == nullptr) {
		spdlog::error("VulkanRenderer::RegisterTexture: Specified GPUTexture is not a Vulkan texture");
		throw std::runtime_error("VulkanRenderer::RegisterTexture: Specified GPUTexture is not a Vulkan texture");
		return UINT32_MAX;
	}

	if (this->m_textureIndices.count(textureName) > 0) {
		return this->m_textureIndices[textureName];
	}

	uint32_t nTextureIndex = this->m_loadedTextures.size();

	if (nTextureIndex >= this->m_nMaxTextures) {
		spdlog::error("VulkanRenderer::RegisterTexture: Max texture count reached");
		return UINT32_MAX;
	}

	this->m_loadedTextures.push_back(pTexture);
	this->m_textureIndices[textureName] = nTextureIndex;

	/* Update descriptor set */
	VkDescriptorImageInfo imageInfo = { };
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture->GetImageView();
	imageInfo.sampler = texture->GetSampler();

	VkWriteDescriptorSet descriptorWrite = { };
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = this->m_globalTextureDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = nTextureIndex;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(this->m_device, 1, &descriptorWrite, 0, nullptr);

	return nTextureIndex;
}

/* 
	Loads EXR cubemap and creates 
	a GPUTexture with the Cubemap texture type 
*/
GPUTexture* VulkanRenderer::CreateCubemap(const String filePath, ECubemapLayout layout) {
	/* Load EXR with TinyEXR */
	float* pRGBA = nullptr;
	int nWidth, nHeight;
	const char* pcErr = nullptr;

	std::filesystem::path path = filePath;
	if (!path.is_absolute()) {
		path = std::filesystem::path(GetExecutableDir()) / filePath;
	}

	int nRet = LoadEXR(&pRGBA, &nWidth, &nHeight, path.string().c_str(), &pcErr);
	if (nRet != TINYEXR_SUCCESS) {
		if (pcErr) {
			spdlog::error("VulkanRenderer::CreateCubemap: Error loading EXR {0}", pcErr);
			FreeEXRErrorMessage(pcErr);
		}
		return nullptr;
	}

	/* Calculate each face size */
	int nFaceWidth, nFaceHeight = 0;
	switch (layout) {
		case ECubemapLayout::HORIZONTAL_CROSS: nFaceWidth = nWidth / 4; nFaceHeight = nHeight / 3; break;
		case ECubemapLayout::VERTICAL_CROSS: nFaceWidth = nWidth / 3; nFaceHeight = nHeight / 4; break;
		case ECubemapLayout::HORIZONTAL_STRIP: nFaceWidth = nWidth / 6; nFaceHeight; break;
		case ECubemapLayout::VERTICAL_STRIP: nFaceWidth = nWidth; nFaceHeight = nHeight / 6; break;
	}

	/* Extract 6 faces on a buffer */
	uint32_t nFaceSize = nFaceWidth * nFaceHeight * 4 * sizeof(float); // Width * height * 4 channels * size of float.
	uint32_t nTotalSize = nFaceSize * 6;
	Vector<float> extractedData(nTotalSize / sizeof(float));

	this->ExtractCubemapFaces(pRGBA, nWidth, nHeight, extractedData.data(), nFaceWidth, nFaceHeight, layout);
	free(pRGBA); // We don't need the original EXR.

	/* Create a staging buffer */
	GPUBuffer* stagingBuffer = this->CreateStagingBuffer(extractedData.data(), nTotalSize);
	VulkanBuffer* vkStagingBuffer = dynamic_cast<VulkanBuffer*>(stagingBuffer);

	VkBuffer vkBuffer = vkStagingBuffer->GetBuffer();

	VkImage cubemapImage = nullptr;
	VkDeviceMemory cubemapMemory = nullptr;
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

	/* Image create info with CREATE_CUBE_COMPATIBLE flag */
	VkImageCreateInfo imageInfo = { };
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { static_cast<uint32_t>(nFaceWidth), static_cast<uint32_t>(nFaceHeight), 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 6; // 6 faces
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	/* Create image */
	vkCreateImage(this->m_device, &imageInfo, nullptr, &cubemapImage);

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(this->m_device, cubemapImage, &memReqs);

	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = this->FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(this->m_device, &allocInfo, nullptr, &cubemapMemory);
	vkBindImageMemory(this->m_device, cubemapImage, cubemapMemory, 0);
	this->TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
	
	/* Copy from our staging buffer to our image */
	VkCommandBuffer cmdBuff = this->BeginSingleTimeCommandBuffer();

	Vector<VkBufferImageCopy> regions(6);
	for (uint32_t i = 0; i < 6; i++) {
		regions[i].bufferOffset = i * nFaceSize;
		regions[i].bufferRowLength = 0;
		regions[i].bufferImageHeight = 0;
		regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[i].imageSubresource.mipLevel = 0;
		regions[i].imageSubresource.baseArrayLayer = i;
		regions[i].imageSubresource.layerCount = 1;
		regions[i].imageOffset = { 0, 0, 0 };
		regions[i].imageExtent = { static_cast<uint32_t>(nFaceWidth), static_cast<uint32_t>(nFaceHeight), 1 };
	}

	vkCmdCopyBufferToImage(cmdBuff, vkBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions.data());

	this->EndSingleTimeCommandBuffer(cmdBuff);

	this->TransitionImageLayout(cubemapImage, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);
	delete stagingBuffer;
	stagingBuffer = nullptr;

	/* Create our cubemap image view */
	VkImageViewCreateInfo viewInfo = { };
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = cubemapImage;
	viewInfo.format = format;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 6;

	VkImageView imageView;
	vkCreateImageView(this->m_device, &viewInfo, nullptr, &imageView);

	VkSampler sampler = this->CreateSampler();
	VulkanTexture* texture = new VulkanTexture(
		this->m_device, 
		this->m_physicalDevice, 
		cubemapImage, 
		cubemapMemory, 
		static_cast<uint32_t>(memReqs.size),
		imageView, 
		sampler, 
		ETextureType::CUBEMAP
	);

	return texture;
} 

/*
	Extract the cubemap faces and copy each of them
*/
void VulkanRenderer::ExtractCubemapFaces(
	const float* pcSrcRGBA,
	int nSrcWidth,
	int nSrcHeight,
	float* pDstData,
	int nFaceWidth,
	int nFaceHeight,
	ECubemapLayout layout
) {
	struct FaceOffset { int x, y; };
	FaceOffset offsets[6] = { };

	switch (layout) {
	case ECubemapLayout::HORIZONTAL_CROSS:
		/*
			Layout:
				[  ][+Y][  ][  ]
				[-X][+Z][+X][-Z]
				[  ][-Y][  ][  ]
		*/
		offsets[0] = { 2 * nFaceWidth, nFaceHeight }; // +X
		offsets[1] = { 0, nFaceHeight }; // -X
		offsets[2] = { nFaceWidth, 0 }; // +Y
		offsets[3] = { nFaceWidth, 2 * nFaceHeight }; // -Y
		offsets[4] = { nFaceWidth, nFaceHeight }; // +Z
		offsets[5] = { 3 * nFaceWidth, nFaceHeight }; // -Z
		
		break;
	case ECubemapLayout::VERTICAL_CROSS:
		/*
			Layout:
			[  ][+Y][  ]
			[-X][+Z][+X]
			[  ][-Y][  ]
			[  ][-Z][  ]
		*/
		offsets[0] = { 2 * nFaceWidth, nFaceHeight }; // +X
		offsets[1] = { 0, nFaceHeight }; // -X
		offsets[2] = { nFaceWidth, 0 }; // +Y
		offsets[3] = { nFaceWidth, 2 * nFaceHeight }; // -Y
		offsets[4] = { nFaceWidth, nFaceHeight }; // +Z
		offsets[5] = { nFaceWidth, 3 * nFaceHeight }; // -Z

		break;
	case ECubemapLayout::HORIZONTAL_STRIP:
		for (int i = 0; i < 6; i++) offsets[i] = { i * nFaceWidth, 0 };
		break;
	case ECubemapLayout::VERTICAL_STRIP:
		for (int i = 0; i < 6; i++) offsets[i] = { 0, i * nFaceHeight };
		break;
	}

	/* Copy each face */
	for (int nFace = 0; nFace < 6; nFace++) {
		float* pDstFace = pDstData + (nFace * nFaceWidth * nFaceHeight * 4);
		
		for (int y = 0; y < nFaceHeight; y++) {
			for (int x = 0; x < nFaceWidth; x++) {
				int nSrcX = offsets[nFace].x + x;
				int nSrcY = offsets[nFace].y + y;
				int nSrcIdX = (nSrcY * nSrcWidth + nSrcX) * 4;
				int nDstIdX = ( y * nFaceWidth + x ) * 4;

				for (int c = 0; c < 4; c++) {
					pDstFace[nDstIdX + c] = pcSrcRGBA[nSrcIdX + c];
				}
			}
		}
	}
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

	VkCommandBuffer commandBuffer = this->m_commandBuffers[this->m_nCurrentFrameIndex];

	VulkanBuffer* vkBuff = dynamic_cast<VulkanBuffer*>(buffer);
	
	/* Get our VkBuffer, VkDeviceMemory and our buffer size */
	VkBuffer buff = vkBuff->GetBuffer();
	uint32_t nSize = vkBuff->GetSize();

	/* Bind our vertex buffer and draw it */
	VkBuffer vertexBuffers[] = { buff };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdDraw(commandBuffer, nSize / sizeof(Vertex), 1, 0, 0);

	return true;
}

/* Bind and draw index buffer */
bool VulkanRenderer::DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo) {
	if (dynamic_cast<VulkanBuffer*>(vbo) == nullptr || dynamic_cast<VulkanBuffer*>(ibo) == nullptr) {
		spdlog::error("VulkanRenderer::DrawIndexBuffer: Specified VBO or IBO is not a Vulkan buffer");
		throw std::runtime_error("VulkanRenderer::DrawIndexBuffer: Specified VBO or IBO is not a Vulkan buffer");
		return false;
	}

	VkCommandBuffer commandBuffer = this->m_commandBuffers[this->m_nCurrentFrameIndex];

	VulkanBuffer* vkVBO = dynamic_cast<VulkanBuffer*>(vbo);
	VulkanBuffer* vkIBO = dynamic_cast<VulkanBuffer*>(ibo);

	if (vkVBO->m_bufferType != EBufferType::VERTEX_BUFFER || vkIBO->m_bufferType != EBufferType::INDEX_BUFFER) {
		spdlog::error("VulkanRenderer::DrawIndexBuffer: Specified VBO or IBO is not of the required type");
		throw std::runtime_error("VulkanRenderer::DrawIndexBuffer: Specified VBO or IBO is not of the required type");
		return false;
	}

	VkBuffer vertexBuffers[] = { vkVBO->GetBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, vkIBO->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

	uint32_t nIndexCount = static_cast<uint32_t>(vkIBO->GetSize() / sizeof(uint16_t));
	
	vkCmdDrawIndexed(commandBuffer, nIndexCount, 1, 0, 0, 0);

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
void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t nLayerCount) {
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
	barrier.subresourceRange.layerCount = nLayerCount;

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
	/* From COLOR_ATTACHMENT_OPTIMAL to TRANSFER_SRC_OPTIMAL */
	else if(oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	/* From DST_OPTIMAL to PRESENT_SRC */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = 0;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
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
	Vector<VkQueueFamilyProperties> queueFamilies(nQueueFamilyCount);
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
	Vector<VkExtensionProperties> extensions(nExtensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &nExtensionCount, extensions.data());

	/* Make a copy of the 'deviceExtensions' vector to a set */
	std::set<String> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

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
Vector<const char*> VulkanRenderer::GetRequiredExtensions() {
	/* Enum GLFW extensions */
	uint32_t nGlfwExtensions = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&nGlfwExtensions);

	/* Store GLFW extensions on a vector */
	Vector<const char*> extensions(glfwExtensions, glfwExtensions + nGlfwExtensions);

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
	Vector<VkLayerProperties> availableLayers(nLayerCount);
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

	vkWaitForFences(this->m_device, 1, &this->m_inFlightFences[this->m_nCurrentFrameIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(this->m_device, 1, &this->m_inFlightFences[this->m_nCurrentFrameIndex]);

	uint32_t nImageIndex;
	vkAcquireNextImageKHR(this->m_device, this->m_sc, UINT64_MAX, this->m_imageAvailableSemaphores[this->m_nCurrentFrameIndex], VK_NULL_HANDLE, &nImageIndex);

	vkResetCommandBuffer(this->m_commandBuffers[this->m_nCurrentFrameIndex], 0);
	this->RecordCommandBuffer(nImageIndex);

	VkSemaphore waitSemaphores[] = { this->m_imageAvailableSemaphores[this->m_nCurrentFrameIndex] };
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSemaphore signalSemaphores[] = { this->m_renderFinishedSemaphores[this->m_nCurrentFrameIndex] };

	/* Setup our submit info */
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.pCommandBuffers = &this->m_commandBuffers[this->m_nCurrentFrameIndex];
	submitInfo.commandBufferCount = 1;

	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.signalSemaphoreCount = 1;

	if (vkQueueSubmit(this->m_graphicsQueue, 1, &submitInfo, this->m_inFlightFences[this->m_nCurrentFrameIndex]) != VK_SUCCESS) {
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
	this->m_nCurrentFrameIndex = (nImageIndex + 1) % this->m_nImageCount;
}
