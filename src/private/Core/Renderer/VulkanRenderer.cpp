#include "Core/Renderer/VulkanRenderer.h"

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
}

/* Renderer init method */
void VulkanRenderer::Init() {
	Renderer::Init();

	this->CreateInstance();
	this->SetupDebugMessenger();
	this->CreateSurface();
	this->PickPhysicalDevice();
	this->CreateLogicalDevice();
	this->CreateSwapChain();
	this->CreateImageViews();
	this->CreateRenderPass();
	this->CreateFrameBuffers();
	this->CreateGraphicsPipeline();
	this->CreateVertexBuffer();
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
	appInfo.apiVersion = VK_API_VERSION_1_0;
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

	/* Logical device create info */
	VkDeviceCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();

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
		spdlog::error("CrateLogicalDevice: Couldn't create logical device");
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
	if (details.capabilities.maxImageCount > 0 && details.capabilities.maxImageCount > nImageCount) {
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
		VkImageViewCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = this->m_scImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = this->m_surfaceFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(this->m_device, &createInfo, nullptr, &this->m_imageViews[i]) != VK_SUCCESS) {
			spdlog::error("CreateImageViews: Failed to create image view");
			throw std::runtime_error("CreateImageViews: Failed to create image view");
			return;
		}
	}

	spdlog::debug("CreateImageViews: All image views created");
}

/* 
	Creation of our render pass with its attachments and subpasses
*/
void VulkanRenderer::CreateRenderPass() {
	/* Our color attachment description */
	VkAttachmentDescription attachmentDesc = { };
	attachmentDesc.format = this->m_surfaceFormat;
	attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	/* A reference to our attachment. (We'll use the 0 because we only have one) */
	VkAttachmentReference attachReference = { };
	attachReference.attachment = 0;
	attachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	/* Our sub-pass description */
	VkSubpassDescription subPass = { };
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &attachReference;
	
	/* Our sub-pass dependency */
	VkSubpassDependency dependency = { };
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstSubpass = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	/* Store the descriptors on arrays */
	VkAttachmentDescription attachments[] = { attachmentDesc };
	VkSubpassDescription subPasses[] = { subPass };
	VkSubpassDependency dependencies[] = { dependency };

	/* Render pass creation info */
	VkRenderPassCreateInfo createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = 1;
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

/* 
	Create a frame buffer per each image view
*/
void VulkanRenderer::CreateFrameBuffers() {
	/* We'll have same frame buffers as image views */
	this->m_frameBuffers.resize(this->m_imageViews.size());

	/* For each image view, create a frame buffer */
	for (uint32_t i = 0; i < this->m_frameBuffers.size(); i++) {
		VkImageView attachments[] = {
			this->m_imageViews[i]
		};

		/* Our frame buffer creation info */
		VkFramebufferCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.attachmentCount = 1;
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
	VkVertexInputAttributeDescription attribs[2] = {};
	attribs[0].binding = 0;
	attribs[0].location = 0;
	attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribs[0].offset = offsetof(Vertex, Vertex::position);

	attribs[1].binding = 0;
	attribs[1].location = 1;
	attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attribs[1].offset = offsetof(Vertex, Vertex::color);

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
	vertexInfo.vertexAttributeDescriptionCount = 2;
	vertexInfo.pVertexAttributeDescriptions = attribs;

	/* Our input assembly creation info */
	VkPipelineInputAssemblyStateCreateInfo inputInfo = { };
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	/* Defining our viewport and scissor */
	this->m_viewport = { };
	this->m_viewport.width = this->m_scExtent.width;
	this->m_viewport.height = this->m_scExtent.height;
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
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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

/* Creation of our triangle vertex buffer */
void VulkanRenderer::CreateVertexBuffer() {
	/* Define our vertices */
	std::vector<Vertex> vertices = {
		{{.0f, -.5f, 0.f}, {1.f, 0.f, 0.f, 1.f}},
		{{.5f, .5f, 0.f}, {0.f, 1.f, 0.f, 1.f}},
		{{-.5f, .5f, 0.f}, {0.f, 0.f, 1.f, 1.f}},
	};

	/* Buffer create info */
	VkBufferCreateInfo bufferInfo = { };
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(this->m_device, &bufferInfo, nullptr, &this->m_vertexBuffer) != VK_SUCCESS) {
		spdlog::error("CreateVertexBuffer: Failed creating vertex buffer");
		throw std::runtime_error("CraeteVertexBuffer: Failed creating vertex buffer");
		return;
	}

	/* Memory requirements */
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(this->m_device, this->m_vertexBuffer, &requirements);

	/* 
		Memory allocation 
		Memory type index property flags: 
			- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: CPU can access directly from memory
			- VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: GPY can directly access to the data without any extra step
	*/
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	

	if (vkAllocateMemory(this->m_device, &allocInfo, nullptr, &this->m_vertexMemory) != VK_SUCCESS) {
		spdlog::error("CreateVertexBuffer: Error allocating memory");
		throw std::runtime_error("CreateVertexBuffer: Error allocating memory");
		return;
	}

	/* Map memory and copy vertices */
	void* pData = nullptr;
	vkMapMemory(this->m_device, this->m_vertexMemory, 0, bufferInfo.size, 0, &pData);
	memcpy(pData, vertices.data(), bufferInfo.size);
	vkUnmapMemory(this->m_device, this->m_vertexMemory);

	/* Bind buffer memory */
	vkBindBufferMemory(this->m_device, this->m_vertexBuffer, this->m_vertexMemory, 0);
	
	spdlog::debug("CreateVertexBuffer: Vertex buffer created");
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

}