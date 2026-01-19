#pragma once
#include "Core/Renderer/Swapchain.h"

#include "Core/Renderer/Vulkan/VulkanDevice.h"
#include "Core/Renderer/Vulkan/VulkanImageView.h"
#include "Core/Renderer/Vulkan/VulkanHelpers.h"

struct SwapchainSupportDetails;

class VulkanSwapchain : public Swapchain {
public:
	using Ptr = Ref <VulkanSwapchain>;

	explicit VulkanSwapchain(Ref<VulkanDevice> device, VkSurfaceKHR surface);
	~VulkanSwapchain() override;

	void Create(const SwapchainCreateInfo& createInfo) override;

	uint32_t AcquireNextImage(
		uint64_t nTimeout = UINT64_MAX,
		void* pSignalSemaphore = nullptr,
		void* pSignalFence = nullptr
	) override;

	bool Present(
		uint32_t nImageIndex,
		const Vector<void*>& pWaitSemaphores = {}
	) override;

	void Rebuild(uint32_t nNewWidth, uint32_t nNewHeight) override;

	uint32_t GetImageCount() const override { return this->m_nImageCount; };

	Ref<GPUTexture> GetImage(uint32_t nIndex) const override { return this->m_images[nIndex]; }
	Ref<ImageView> GetImageView(uint32_t nIndex) const { return this->m_imageViews[nIndex]; }

	Ref<GPUTexture> GetDepthImage() const override { return this->m_depthImage; }
	Ref<ImageView> GetDepthImageView() const override { return this->m_depthImageView; }
	GPUFormat GetDepthFormat() const override { return this->m_depthFormat; }

	Extent2D GetExtent() const override { return Extent2D { this->m_extent.width, this->m_extent.height }; }

	bool NeedsRebuild() const override { return this->m_bNeedsRebuild; }

	static Ptr 
	CreateShared(Ref<VulkanDevice> device, VkSurfaceKHR surface) {
		return CreateRef<VulkanSwapchain>(device, surface);
	}

private:
	Ref<VulkanDevice> m_device;
	VkSwapchainKHR m_swapchain;
	VkSurfaceKHR m_surface;

	GLFWwindow* m_pWindow;
	
	Vector<Ref<GPUTexture>> m_images;
	Vector<Ref<ImageView>> m_imageViews;

	Ref<GPUTexture> m_depthImage;
	Ref<ImageView> m_depthImageView;

	VkExtent2D m_extent;
	GPUFormat m_format;
	GPUFormat m_depthFormat;

	EPresentMode m_presentMode;
	uint32_t m_nImageCount;
	bool m_bNeedsRebuild;

	SwapchainCreateInfo m_createInfo; // Cached for rebuilds

	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice);

	VkSurfaceFormatKHR ChooseSurfaceFormat(const Vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR ChooseSwapPresentMode(const Vector<VkPresentModeKHR>& presentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};