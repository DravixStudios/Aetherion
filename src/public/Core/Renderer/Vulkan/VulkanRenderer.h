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
#include "Core/Renderer/Vulkan/VulkanRingBuffer.h"
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

class SceneManager;

class VulkanRenderer : public Renderer {
public:
	VulkanRenderer();
	
	void Init() override;
	void Update() override;
private:
	SceneManager* m_sceneMgr;

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
	uint32_t m_nImageCount;

	VkRenderPass m_renderPass;
	std::vector<VkFramebuffer> m_frameBuffers;

	VkImage m_depthImage;
	VkImageView m_depthImageView;

	VkCommandPool m_commandPool;
	VkCommandBuffer m_commandBuffer;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	VkSemaphore m_imageAvailable;
	VkSemaphore m_renderFinished;
	VkFence m_fence;

	VkSampleCountFlagBits m_multisampleCount;
	VkImage m_colorImage;
	VkDeviceMemory m_colorImageMemory;
	VkImageView m_colorImageView;

	std::vector<VkDescriptorSet> m_descriptorSets;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool m_descriptorPool;

	uint32_t m_nMaxDescriptorSetSamplers;
	uint32_t m_nMaxPerStageDescriptorSamplers;
	uint32_t m_nMaxTextures;

	/* Main methods */
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CheckDescriptorIndexingSupport();
	void QueryDeviceLimits();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateCommandPool();
	void CreateColorResources();
	void CreateDepthResources();
	void CreateFrameBuffers();
	void CreateDescriptorSetLayout();
	void CreateDescriptorPool();
	void AllocateAndWriteDescriptorSets();
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

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);

	uint32_t CreateImage(
		uint32_t nWidth, 
		uint32_t nHeight, 
		VkFormat format, 
		VkSampleCountFlagBits multisampleCount,
		VkImageTiling tiling, 
		VkImageUsageFlags usageFlags, 
		VkMemoryPropertyFlags propertyFlags,
		VkImage& image,
		VkDeviceMemory& memory
	);

public:
	/* Memory methods */
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	GPUBuffer* CreateBuffer(const void* pData, uint32_t nSize, EBufferType bufferType) override;
	GPUBuffer* CreateVertexBuffer(const std::vector<Vertex>& vertices) override;
	GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize) override;
	GPUTexture* CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) override;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkSampler CreateSampler();
private:
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t nWidth, uint32_t nHeight);
	bool DrawVertexBuffer(GPUBuffer* buffer) override;

	WVP m_wvp;
	GPURingBuffer* m_wvpBuff;

	VkSampleCountFlagBits GetMaxUsableSampleCount();

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkCommandBuffer BeginSingleTimeCommandBuffer();
	void EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer);

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
