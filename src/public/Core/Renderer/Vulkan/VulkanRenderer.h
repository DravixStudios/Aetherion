#pragma once
#include "Core/Renderer/Renderer.h"
#include <set>

#define GLFW_INCLUDE_VULKAN
#ifdef __APPLE__
#define VK_USE_PLATFORM_MACOS_MVK
#endif // __APPLE__
#include <GLFW/glfw3.h>

class VulkanDevice;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities = { };
	Vector<VkSurfaceFormatKHR> formats;
	Vector<VkPresentModeKHR> presentModes;
};

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
	Vector<VkImage> m_scImages;
	Vector<VkImageView> m_imageViews;
	uint32_t m_nImageCount;

	/* Framebuffers */
	VkFramebuffer m_gbufferFramebuffer;
	VkFramebuffer m_lightingFramebuffer;
	Vector<VkFramebuffer> m_scFrameBuffers;
	Vector<VkFramebuffer> m_skyboxFrameBuffers;

	/* Render passes */
	VkRenderPass m_geometryRenderPass;
	VkRenderPass m_lightingRenderPass;
	VkRenderPass m_skyboxRenderPass;

	/* Depth resources */
	VkImage m_depthImage;
	VkImageView m_depthImageView;
	VkSampler m_depthSampler;
	VkDeviceMemory m_depthMemory;

	VkImage m_depthResolveImage;
	VkImageView m_depthResolveImageView;
	VkDeviceMemory m_depthResolveMemory;

	static constexpr const char* CLASS_NAME = "VulkanRenderer";

	explicit VulkanRenderer();
	~VulkanRenderer() override;

	void Create(GLFWwindow* pWindow) override;

	Ref<Device> CreateDevice() override;

	static Ptr
	CreateShared() {
		return CreateRef<VulkanRenderer>();
	}

	/* Command pool and buffers */
	VkCommandPool m_commandPool;
	Vector<VkCommandBuffer> m_commandBuffers;

	/* Pipelines */
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;

	VkPipelineLayout m_gbuffPipelineLayout;
	VkPipeline m_gbuffPipeline;

	VkPipelineLayout m_lightingPipelineLayout;
	VkPipeline m_lightingPipeline;

	VkPipelineLayout m_skyboxPipelineLayout;
	VkPipeline m_skyboxPipeline;

	VkPipelineLayout m_cullingPipelineLayout;
	VkPipeline m_cullingPipeline;

	/* IBL Generation Pipelines */
	VkPipelineLayout m_irradiancePipelinelayout;
	VkPipeline m_irradiancePipeline;
	VkRenderPass m_irradianceRenderPass;

	VkPipelineLayout m_prefilterPipelineLayout;
	VkPipeline m_prefilterPipeline;
	VkRenderPass m_prefilterRenderPass;

	VkPipelineLayout m_brdfPipelineLayout;
	VkPipeline m_brdfPipeline;
	VkRenderPass m_brdfRenderPass;

	VkViewport m_viewport;
	VkRect2D m_scissor;

	/* Sync objects */
	Vector<VkSemaphore> m_imageAvailableSemaphores;
	Vector<VkSemaphore> m_renderFinishedSemaphores;
	Vector<VkFence> m_inFlightFences;

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
	VkDeviceMemory m_colorMemory;
	VkDeviceMemory m_normalMemory;
	VkDeviceMemory m_ormMemory;
	VkDeviceMemory m_emissiveMemory;
	VkDeviceMemory m_positionMemory;
	VkDeviceMemory m_colorResolveMemory;
	VkDeviceMemory m_normalResolveMemory;
	VkDeviceMemory m_ormResolveMemory;
	VkDeviceMemory m_emissiveResolveMemory;
	VkDeviceMemory m_positionResolveMemory;

	bool m_bEnableValidationLayers;
	VkInstance m_instance;
	
	VkSurfaceKHR m_surface;

	VkDebugUtilsMessengerEXT m_debugMessenger;

	VkPhysicalDevice m_physicalDevice;

	bool m_framebufferResized;

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
	void CreateIrradianceRenderPass();
	void CreatePrefilterRenderPass();
	void CreateBRDFRenderPass();
	void CreateCubeMesh();
	void CreateCommandPool();
	void CreateGBufferResources();
	void CreateGlobalGeometryBuffers();
	void CreateDepthResources();
	void CreateLightingResources();
	void CreateGBufferFrameBuffer();
	void CreateLightingFrameBuffer();
	void CreateSkyboxFrameBuffer();
	void CreateDescriptorSetLayout(); 
	void CreateLightingDescriptorSetLayout();
	void CreateSkyboxDescriptorSetLayout();
	void CreateCullingDescriptorSetLayout();
	void CreateDescriptorPool();
	void CreateLightingDescriptorPool();
	void CreateSkyboxDescriptorPool();
	void CreateCullingDescriptorPool();
	void AllocateDescriptorSets();
	void AllocateLightingDescriptorSets();
	void AllocateSkyboxDescriptorSets();
	void AllocateCullingDescriptorSets();
	void WriteDescriptorSets();
	void CreateIndirectBuffers();
	void GenerateIrradianceMap();
	void GeneratePrefilterMap();
	void GenerateBRDFLUT();
	void WriteLightDescriptorSets();
	void WriteSkyboxDescriptorSets();
	void WriteCullingDescriptorSets();
	void CreateCommandBuffer();
	void CleanupSwapChain();
	void RecreateSwapChain();
	void UpdateViewportAndScissor();
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
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
	void CreateIrradiancePipeline();
	void CreatePrefilterPipeline();
	void CreateCullingPipeline();
	void CreateBRDFPipeline();
	void CreateSyncObjects();
	void RecordCommandBuffer(uint32_t nImageIndex);
	void DispatchComputeCulling(VkCommandBuffer commandBuff);
	void UpdateInstanceData(uint32_t nFrameIndex);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const Vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseSwapPresentMode(const Vector<VkPresentModeKHR>& presentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	String ReadShader(const String& sFile);
	Vector<uint32_t> CompileShader(String shader, String filename, shaderc_shader_kind kind);
	VkShaderModule CreateShaderModule(Vector<uint32_t>& shaderCode);

	VkFormat FindSupportedFormat(const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
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
	GPUBuffer* CreateIndexBuffer(const Vector<uint16_t>& indices) override;
	GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize) override;
	GPUTexture* CreateTexture(
		GPUBuffer* pBuffer,
		uint32_t nWidth,
		uint32_t nHeight, 
		GPUFormat format
	) override;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkImageView CreateCubemapImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t nMipLevels);
	VkSampler CreateSampler();
	VkSampler CreateCubemapSampler(uint32_t nMipLevels);

	uint32_t GetTextureIndex(String& textureName);
	uint32_t RegisterTexture(const String& textureName, GPUTexture* pTexture);

	void UploadMeshToGlobalBuffers(const Vector<Vertex>& vertices, const Vector<uint16_t>& indices, Mesh::SubMesh& outSubMesh) override;
	
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

	void ConvertEquirectangularToHorizontalCross(
		const float* pcSrcRGBA,
		int nSrcWidth,
		int nSrcHeight,
		float* pDstData,
		int nFaceWidth,
		int nFaceSize
	) override;
	
	void ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 planes[6]);

	bool DrawVertexBuffer(GPUBuffer* buffer) override;
	bool DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo) override;
private:
	Vector<GPUTexture*> m_loadedTextures;
	std::map<String, uint32_t> m_textureIndices;

	bool CheckDeviceExtensionSupport(const VkPhysicalDevice& physicalDevice);
	SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice);

	bool CheckValidationLayersSupport();
	Vector<const char*> GetRequiredExtensions();

	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& messengerInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
		VkDebugUtilsMessageTypeFlagsEXT type, 
		const VkDebugUtilsMessengerCallbackDataEXT* pcData, 
		void* pvUserData
	);

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
		VkAllocationCallbacks* pAllocator, 
		VkDebugUtilsMessengerEXT* pDebugMessenger
	);

};