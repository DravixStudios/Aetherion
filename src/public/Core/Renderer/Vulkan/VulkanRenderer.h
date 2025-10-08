#pragma once
#include <iostream>
#include <cstring>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <shaderc/shaderc.hpp>

#include "Core/Renderer/Renderer.h"
#include "Core/Renderer/Vulkan/VulkanBuffer.h"
#include "Core/Renderer/Vulkan/VulkanTexture.h"
#include "Core/Renderer/GPUFormat.h"
#include "Utils.h"

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS 1
#else
#define ENABLE_VALIDATION_LAYERS 0
#endif // NDEBUG

/* Forward declarations */
struct QueueFamilyIndices;
struct SwapChainSupportDetails;

class VulkanRenderer : public Renderer {
public:
	VulkanRenderer();
	
	void Init() override;
	void Update() override;
private:
	/* Debug messenger members */
	bool m_bEnableValidationLayers;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	/* Main Vulkan members */
	VkInstance m_vkInstance;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSwapchainKHR m_sc;
	VkExtent2D m_scExtent;
	VkFormat m_surfaceFormat;
	std::vector<VkImage> m_scImages;
	std::vector<VkImageView> m_imageViews;

	VkRenderPass m_renderPass;
	std::vector<VkFramebuffer> m_frameBuffers;

	VkCommandPool m_commandPool;
	VkCommandBuffer m_commandBuffer;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	VkSemaphore m_imageAvailable;
	VkSemaphore m_renderFinished;
	VkFence m_fence;

	/* Main methods */
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateGraphicsPipeline();
	void CreateSyncObjects();
	void RecordCommandBuffer(uint32_t nImageIndex);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	std::string ReadShader(const std::string& sFile);
	std::vector<uint32_t> CompileShader(std::string shader, std::string filename, shaderc_shader_kind kind);
	VkShaderModule CreateShaderModule(std::vector<uint32_t>& shaderCode);

	/* Memory methods */
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	GPUBuffer* CreateVertexBuffer(const std::vector<Vertex>& vertices) override;
	GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize) override;
	GPUTexture* CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) override;
	bool DrawVertexBuffer(GPUBuffer* buffer) override;
	GPUBuffer* m_buffer;

	/* Physical device methods */
	bool IsDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	
	/* Debug messenger */
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity,
		VkDebugUtilsMessageTypeFlagsEXT msgType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCbData,
		void* pvUserData
	);
	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance, 
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		VkAllocationCallbacks* pAllocator, 
		VkDebugUtilsMessengerEXT* pDebugMessenger
	);

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator
	);
	
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	bool CheckValidationLayersSupport();

	/* Extensions */
	std::vector<const char*> GetRequiredExtensions();
	
};
