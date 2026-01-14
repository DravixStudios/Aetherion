#include "Core/Renderer/Vulkan/VulkanRenderer.h"

Vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VulkanRenderer::VulkanRenderer() 
	: m_instance(VK_NULL_HANDLE), m_bEnableValidationLayers(false), m_debugMessenger(VK_NULL_HANDLE),
	  m_pWindow(nullptr), m_surface(VK_NULL_HANDLE) {}

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