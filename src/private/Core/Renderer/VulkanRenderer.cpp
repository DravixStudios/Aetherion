#include "Core/Renderer/VulkanRenderer.h"

/* Validation layers (hard-coded) */
std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

/* Device extensions (hard-coded) */
std::vector<std::string> deviceExtensions = {
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
	this->m_debugMessenger = nullptr;
}

/* Renderer init method */
void VulkanRenderer::Init() {
	Renderer::Init();

	this->CreateInstance();
	this->SetupDebugMessenger();
	this->CreateSurface();
	this->PickPhysicalDevice();
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

	}
}

/* Check if the physical device is suitable */
bool VulkanRenderer::IsDeviceSuitable(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions) {
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