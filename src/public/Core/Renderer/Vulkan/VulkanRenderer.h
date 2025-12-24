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

#define INVALID_INDEX 0xFFFFFFFFu

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

	/* Swap chain & Swap chain resources */
	VkSwapchainKHR m_sc;
	VkExtent2D m_scExtent;
	VkFormat m_surfaceFormat;
	std::vector<VkImage> m_scImages;
	std::vector<VkImageView> m_imageViews;
	uint32_t m_nImageCount;

	/* Framebuffers */
	VkFramebuffer m_gbufferFramebuffer;
	VkFramebuffer m_lightingFramebuffer;
	std::vector<VkFramebuffer> m_scFrameBuffers;
	std::vector<VkFramebuffer> m_skyboxFrameBuffers;

	/* Render passes */
	VkRenderPass m_geometryRenderPass;
	VkRenderPass m_lightingRenderPass;
	VkRenderPass m_skyboxRenderPass;

	/* Depth resources */
	VkImage m_depthImage;
	VkImageView m_depthImageView;
	VkSampler m_depthSampler;

	VkImage m_depthResolveImage;
	VkImageView m_depthResolveImageView;

	/* ScreenQuad resources */
	GPUBuffer* m_sqVBO;
	GPUBuffer* m_sqIBO;

	/* Command pool and buffers */
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	/* Pipelines */
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkPipelineLayout m_gbuffPipelineLayout;
	VkPipeline m_gbuffPipeline;

	VkPipelineLayout m_lightingPipelineLayout;
	VkPipeline m_lightingPipeline;

	VkPipelineLayout m_skyboxPipelineLayout;
	VkPipeline m_skyboxPipeline;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	/* Sync objects */
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	uint32_t m_nCurrentFrameIndex;

	VkSampleCountFlagBits m_multisampleCount;

	/* G-Buffers */
	VkImage m_colorBuffer;
	VkImage m_normalBuffer;
	VkImage m_ormBuffer;
	VkImage m_emissiveBuffer;
	VkImage m_positionBuffer;
	VkImage m_colorResolveBuffer;
	VkImage m_normalResolveBuffer;
	VkImage m_ormResolveBuffer;
	VkImage m_emissiveResolveBuffer;
	VkImage m_positionResolveBuffer;

	/* G-Buffer image view */
	VkImageView m_colorBuffView;
	VkImageView m_normalBuffView;
	VkImageView m_ormBuffView;
	VkImageView m_emissiveBuffView;
	VkImageView m_positionBuffView;
	VkImageView m_colorResolveBuffView;
	VkImageView m_normalResolveBuffView;
	VkImageView m_ormResolveBuffView;
	VkImageView m_emissiveResolveBuffView;
	VkImageView m_positionResolveBuffView;
	
	/* G-Buffer samplers */
	VkSampler m_baseColorSampler;
	VkSampler m_normalSampler;
	VkSampler m_ormSampler;
	VkSampler m_emissiveSampler;
	VkSampler m_positionSampler;

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

	VkDescriptorSetLayout m_skyboxDescriptorSetLayout;
	VkDescriptorPool m_skyboxDescriptorPool;
	std::vector<VkDescriptorSet> m_skyboxDescriptorSets;

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
	void CreateSkyboxRenderPass();
	void CreateCommandPool();
	void CreateGBufferResources();
	void CreateDepthResources();
	void CreateLightingResources();
	void CreateGBufferFrameBuffer();
	void CreateLightingFrameBuffer();
	void CreateSkyboxFrameBuffer();
	void CreateDescriptorSetLayout(); 
	void CreateLightingDescriptorSetLayout();
	void CreateSkyboxDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateLightingDescriptorPool();
	void CreateSkyboxDescriptorPool();
	void AllocateDescriptorSets();
	void AllocateLightingDescriptorSets();
	void AllocateSkyboxDescriptorSets();
	void WriteDescriptorSets();
	void WriteLightDescriptorSets();
	void WriteSkyboxDescriptorSets();
	void CreateCommandBuffer();
	VkPipeline CreateGraphicsPipeline(
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
		VkPushConstantRange* pPushConstantRanges = nullptr,
		uint32_t nPushConstantCount = 0
	);
	void CreateGBufferPipeline();
	void CreateLightingPipeline();
	void CreateSkyboxPipeline();
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
	GPUTexture* CreateTexture(
		GPUBuffer* pBuffer,
		uint32_t nWidth,
		uint32_t nHeight, 
		GPUFormat format
	) override;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkSampler CreateSampler();

	uint32_t GetTextureIndex(String& textureName);
	uint32_t RegisterTexture(const String& textureName, GPUTexture* pTexture);
	
	GPUTexture* CreateCubemap(const String filePath, ECubemapLayout layout = HORIZONTAL_CROSS) override;
	void ExtractCubemapFaces(
		const float* pcSrcRGBA,
		int nSrcWidth,
		int nSrcHeight,
		float* pDstData,
		int nFaceWidth,
		int nFaceHeight,
		ECubemapLayout layout
	) override;

	bool DrawVertexBuffer(GPUBuffer* buffer) override;
	bool DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo) override;
private:
	Vector<GPUTexture*> m_loadedTextures;
	std::map<String, uint32_t> m_textureIndices;

	/* TODO: Remove this */
	GPUTexture* m_skybox;

	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t nWidth, uint32_t nHeight);

	WVP m_wvp;
	GPURingBuffer* m_wvpBuff;

	VkSampleCountFlagBits GetMaxUsableSampleCount();

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t nLayerCount = 1);
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
