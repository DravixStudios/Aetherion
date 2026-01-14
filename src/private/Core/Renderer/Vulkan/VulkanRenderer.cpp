#include "Core/Renderer/Vulkan/VulkanRenderer.h"
#include "Core/Renderer/Vulkan/VulkanDevice.h"

Vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

Vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#ifdef __APPLE__
	"VK_KHR_portability_subset"
#endif // __APPLE__
};

VulkanRenderer::VulkanRenderer() 
	: m_instance(VK_NULL_HANDLE), m_bEnableValidationLayers(false), m_debugMessenger(VK_NULL_HANDLE),
	  m_pWindow(nullptr), m_surface(VK_NULL_HANDLE), m_physicalDevice(VK_NULL_HANDLE) {}

VulkanRenderer::~VulkanRenderer() {
	if (this->m_instance != VK_NULL_HANDLE) {
		vkDestroyInstance(this->m_instance, nullptr);
		this->m_instance = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::Create(GLFWwindow* pWindow) {
#ifndef NDEBUG
	this->m_bEnableValidationLayers = true;
#endif // NDEBUG

	if (this->m_bEnableValidationLayers && !this->CheckValidationLayersSupport()) {
		Logger::Error("VulkanRenderer::Create: Validation layers enabled but not supported");
		throw std::runtime_error("VulkanRenderer::Create: Validation layers enabled but not supported");
	}

	VkApplicationInfo appInfo = { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pApplicationName = "N.A";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName = "Aetherion Engine";
	appInfo.apiVersion = VK_API_VERSION_1_2;

	Vector<const char*> extensions = this->GetRequiredExtensions();

#ifdef __APPLE__
	extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif // __APPLE__

	size_t nExtensionCount = extensions.size();

	Logger::Debug("VulkanRenderer::Create: Required extension count {}", nExtensionCount);

	VkInstanceCreateInfo instanceInfo = { };
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = nExtensionCount;
	instanceInfo.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
	instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif // __APPLE__

	if (this->m_bEnableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { };
		messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		this->PopulateDebugMessengerCreateInfo(messengerInfo);

		instanceInfo.enabledLayerCount = validationLayers.size();
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
		instanceInfo.pNext = &messengerInfo;
	}
	else {
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.ppEnabledExtensionNames = nullptr;
		instanceInfo.pNext = nullptr;
	}

	VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &this->m_instance), "Failed creating Vulkan instance");

	/* Setup debug messenger */
	if (this->m_bEnableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT messengerInfo = { };
		messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		this->PopulateDebugMessengerCreateInfo(messengerInfo);

		VK_CHECK(this->CreateDebugUtilsMessengerEXT(this->m_instance, &messengerInfo, nullptr, &this->m_debugMessenger), "Failed to setup Vulkan debug messenger");
	}

	/* Create window surface */
	VK_CHECK(
		glfwCreateWindowSurface(
			this->m_instance, 
			this->m_pWindow, 
			nullptr, 
			&this->m_surface
		), "Couldn't create window surface");

	this->PickPhysicalDevice();
	this->CheckDescirptorIndexingSupport();
}

/**
* Picks the most suitable physical device
*/
void 
VulkanRenderer::PickPhysicalDevice() {
	uint32_t nPhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(this->m_instance, &nPhysicalDeviceCount, nullptr);

	Vector<VkPhysicalDevice> physicalDevices(nPhysicalDeviceCount);

	Logger::Debug("VulkanRenderer::PickPhysicalDevice: Available physical device count: {}", nPhysicalDeviceCount);

	for (const VkPhysicalDevice& physicalDevice : physicalDevices) {
		if (this->IsDeviceSuitable(physicalDevice)) {
			this->m_physicalDevice = physicalDevice;
			break;
		}
	}

	if (this->m_physicalDevice == VK_NULL_HANDLE) {
		Logger::Error("VulkanRenderer::PickPhysicalDevice: No suitable device found");
		throw std::runtime_error("VulkanRenderer::PickPhysicalDevice: No suitable device found");
	}

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(this->m_physicalDevice, &deviceProperties);

	/* Check if physical device supports Vulkan 1.2 */
	uint32_t nApiMajor = VK_VERSION_MAJOR(deviceProperties.apiVersion);
	uint32_t nApiMinor = VK_VERSION_MINOR(deviceProperties.apiVersion);

	if (nApiMajor < 1 || (nApiMajor == 1 && nApiMinor < 2)) {
		Logger::Error(
			"VulkanRenderer::PickPhysicalDevice: Selected device does not support Vulkan 1.2 minimum. Found: {}.{}", 
			nApiMajor, 
			nApiMinor
		);

		throw std::runtime_error("VulkanRenderer::PickPhysicalDevice: Vulkan 1.2 required");
	}
}

/**
* Checks if physical device is suitable
* 
* @returns The result of the check
*/
bool 
VulkanRenderer::IsDeviceSuitable(const VkPhysicalDevice& physicalDevice) {
	QueueFamilyIndices indices = this->FindQueueFamilies(physicalDevice);

	bool bExtensionsSupported = this->CheckDeviceExtensionSupport(physicalDevice);

	bool bSwapChainAdequate = false;
	if (bExtensionsSupported) {
		SwapChainSupportDetails swapChainSupport = this->QuerySwapChainSupport(physicalDevice);
		bSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsComplete() && bSwapChainAdequate;
}

/**
* Finds queue family indices
*
* @returns Queue family indices
*/
QueueFamilyIndices 
VulkanRenderer::FindQueueFamilies(const VkPhysicalDevice& physicalDevice) {
	QueueFamilyIndices indices;

	uint32_t nQueueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &nQueueFamilyCount, nullptr);

	Vector<VkQueueFamilyProperties> queueFamilyProperties(nQueueFamilyCount);

	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice,
		&nQueueFamilyCount,
		queueFamilyProperties.data()
	);

	uint32_t i = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilyProperties) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 bPresentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, this->m_surface, &bPresentSupport);

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
* Check if device supports descriptor indexing
*/
void 
VulkanRenderer::CheckDescirptorIndexingSupport() {
	uint32_t nExtensionCount;
	vkEnumerateDeviceExtensionProperties(this->m_physicalDevice, nullptr, &nExtensionCount, nullptr);

	Vector<VkExtensionProperties> availableExtensions(nExtensionCount);
	vkEnumerateDeviceExtensionProperties(this->m_physicalDevice, nullptr, &nExtensionCount, availableExtensions.data());

	bool bDescriptorIndexingSupported = false;

	for (const VkExtensionProperties& extension : availableExtensions) {
		if (strcmp(extension.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0) {
			bDescriptorIndexingSupported = true;
			break;
		}
	}

	if (!bDescriptorIndexingSupported) {
		Logger::Error("VulkanRenderer::CheckDescriptorIndexingSupport: Descriptor indexing is not supported");
		throw std::runtime_error("VulkanRenderer::CheckDescriptorIndexingSupport: Descriptor indexing is not supported");
	}

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
		Logger::Error("VulkanRenderer::CheckDescriptorIndexingSupport: Required descriptor indexing features not available");
		throw std::runtime_error("VulkanRenderer::CheckDescriptorIndexingSupport: Required descriptor indexing features not available");
	}
}

bool
VulkanRenderer::CheckDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice) {
	uint32_t nExtensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &nExtensionCount, nullptr);

	Vector<VkExtensionProperties> extensions(nExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &nExtensionCount, extensions.data());

	std::set<String> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const VkExtensionProperties& extension : extensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

/**
* Check if device supports swap chain
* 
* @returns Result of the check
*/
SwapChainSupportDetails 
VulkanRenderer::QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice) {
	SwapChainSupportDetails details;

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
* Check for validation layers support
* 
* @returns The result of the check
*/
bool 
VulkanRenderer::CheckValidationLayersSupport() {
	uint32_t nLayerCount;
	vkEnumerateInstanceLayerProperties(&nLayerCount, nullptr);

	Vector<VkLayerProperties> availableLayers(nLayerCount);
	vkEnumerateInstanceLayerProperties(&nLayerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool bLayerFound = false;

		for (const VkLayerProperties& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				bLayerFound = true;
				break;
			}
		}

		if (!bLayerFound) return false;
	}

	return true;
}

/**
* Gets a list of required extensions
* 
* @returns A vector of required extension names
*/
Vector<const char*>
VulkanRenderer::GetRequiredExtensions() {
	uint32_t nGlfwExtensions;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&nGlfwExtensions);

	Vector<const char*> extensions(glfwExtensions, glfwExtensions + nGlfwExtensions);

	if (this->m_bEnableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

/**
* Populates debug messenger create info
* 
* @param messengerInfo Reference to Debug messenger create info
*/
void
VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo) {
	messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	   |
								VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerInfo.pfnUserCallback = VulkanRenderer::DebugCallback;
	messengerInfo.pUserData = this;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanRenderer::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pcData,
	void* pvUserData
) {
	VulkanRenderer* renderer = static_cast<VulkanRenderer*>(pvUserData);

	switch (severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			Logger::Debug("Validation layers: {}", pcData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			Logger::Warn("Validation layers: {}", pcData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			Logger::Error("Validation layers: {}", pcData->pMessage);
			break;
	}

	return VK_FALSE;
}

VkResult 
VulkanRenderer::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger
) {
	PFN_vkCreateDebugUtilsMessengerEXT fn = 
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (fn != nullptr) {
		return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}