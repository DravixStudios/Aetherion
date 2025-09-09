#include "Core/Renderer/VulkanRenderer.h"

/* Validation layers (hard-coded) */
std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

/* Constructor */
VulkanRenderer::VulkanRenderer() : Renderer::Renderer() {
	this->m_bEnableValidationLayers = ENABLE_VALIDATION_LAYERS;
	this->m_debugMessenger = nullptr;
}

/* Renderer init method */
void VulkanRenderer::Init() {
	Renderer::Init();

	this->CreateInstance();
	this->SetupDebugMessenger();
	this->CreateSurface();
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