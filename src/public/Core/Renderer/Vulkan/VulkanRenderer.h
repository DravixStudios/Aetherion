#pragma once
#include <iostream>
#include <cstring>
#include <vector>
#include <optional>
#include <set>
#include <map>
#include <algorithm>
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#ifdef __APPLE__
#define VK_USE_PLATFORM_MACOS_MVK
#endif
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

	std::vector<VkFramebuffer> m_scFrameBuffers;

	VkRenderPass m_geometryRenderPass;
	VkRenderPass m_lightingRenderPass;

	VkImage m_depthImage;
	VkImageView m_depthImageView;

	GPUBuffer* m_sqVBO;
	GPUBuffer* m_sqIBO;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkPipelineLayout m_gbuffPipelineLayout;
	VkPipeline m_gbuffPipeline;

	VkPipelineLayout m_lightingPipelineLayout;
	VkPipeline m_lightingPipeline;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	uint32_t m_nCurrentFrameIndex;

	VkSampleCountFlagBits m_multisampleCount;

	/* G-Buffers */
	VkImage m_colorBuffer;
	VkImage m_normalBuffer;
	VkImage m_positionBuffer;
	VkImage m_colorResolveBuffer;
	VkImage m_normalResolveBuffer;
	VkImage m_positionResolveBuffer;

	/* G-Buffer image view */
	VkImageView m_colorBuffView;
	VkImageView m_normalBuffView;
	VkImageView m_positionBuffView;
	VkImageView m_colorResolveBuffView;
	VkImageView m_normalResolveBuffView;
	VkImageView m_positionResolveBuffView;
	
	/* G-Buffer samplers */
	VkSampler m_baseColorSampler;
	VkSampler m_normalSampler;
	VkSampler m_positionSampler;

	VkFramebuffer m_gbufferFramebuffer;
	VkFramebuffer m_lightingFramebuffer;

	/* Descriptor sets */
	std::vector<VkDescriptorSet> m_descriptorSets; // One per frame
	VkDescriptorSetLayout m_wvpDescriptorSetLayout; // For WVP (dynamic)
	VkDescriptorSetLayout m_textureDescriptorSetLayout; // For textures (bindless)
	VkDescriptorPool m_wvpDescriptorPool; 
	VkDescriptorPool m_textureDescriptorPool;
	VkDescriptorSet m_globalTextureDescriptorSet; // Global for all the textures

	VkDescriptorSetLayout m_lightingDescriptorSetLayout;
	VkDescriptorPool m_lightingDescriptorPool;
	std::vector<VkDescriptorSet> m_lightingDescriptorSets;

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
	void CreateGeometryRenderPass();
	void CreateLightingRenderPass();
	void CreateCommandPool();
	void CreateGBufferResources();
	void CreateDepthResources();
	void CreateLightingResources();
	void CreateGBufferFrameBuffer();
	void CreateLightingFrameBuffer();
	void CreateDescriptorSetLayout();
	void CreateLightingDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateLightingDescriptorPool();
	void AllocateDescriptorSets();
	void AllocateLightingDescriptorSets();
	void WriteDescriptorSets();
	void WriteLightDescriptorSets();
	void CreateCommandBuffer();
	VkPipeline CreateGraphicsPipeline(
		const std::string& vertPath,
		const std::string& pixelPath,
		VkRenderPass renderPass,
		VkDescriptorSetLayout* setLayouts,
		uint32_t nSetLayoutCount,
		VkPipelineVertexInputStateCreateInfo vertexInfo,
		VkPipelineRasterizationStateCreateInfo rasterizer,
		VkPipelineMultisampleStateCreateInfo multisampling,
		VkPipelineDepthStencilStateCreateInfo depthStencil,
		VkPipelineColorBlendStateCreateInfo colorBlend,
		VkPipelineLayout* pLayout,
		VkPushConstantRange* pPushConstantRanges = nullptr,
		uint32_t nPushConstantCount = 0
	);
	void CreateGBufferPipeline();
	void CreateLightingPipeline();
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
	GPUBuffer* CreateIndexBuffer(const std::vector<uint16_t>& indices) override;
	GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize) override;
	GPUTexture* CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) override;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkSampler CreateSampler();

	uint32_t GetTextureIndex(std::string& textureName);
	uint32_t RegisterTexture(const std::string& textureName, GPUTexture* pTexture);
private:
	std::vector<GPUTexture*> m_loadedTextures;
	std::map<std::string, uint32_t> m_textureIndices;

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t nWidth, uint32_t nHeight);
	bool DrawVertexBuffer(GPUBuffer* buffer) override;
	bool DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo) override;

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
