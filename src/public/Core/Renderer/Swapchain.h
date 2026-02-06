#pragma once
#include "Core/Containers.h"

#include "Core/Renderer/GPUFormat.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/Rect2D.h"
#include "Core/Renderer/Semaphore.h"
#include "Core/Renderer/Fence.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

enum class EPresentMode {
	IMMEDIATE, // No VSync
	FIFO, // VSync, waits for the next vblank
	FIFO_RELAXED, // Relaxed VSync, it may have tearing
	MAILBOX // Triple buffering, replaces old frames
};

struct SwapchainCreateInfo {
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t nImageCount = 3; // Default triple buffering
	bool bEnableDepthStencil = true;

	/* Swapchain reconstruct (resize) */
	void* pOldSwapchain = nullptr;

	GLFWwindow* pWindow = nullptr;
};

class Swapchain {
public:
	using Ptr = Ref<Swapchain>;

	static constexpr const char* CLASS_NAME = "Swapchain";

	virtual ~Swapchain() = default;

	/**
	* Creates a swapchain
	*
	* @param createInfo Swap chain create info
	*/
	virtual void Create(const SwapchainCreateInfo& createInfo) = 0;

	/**
	* Acquires the next available image for rendering
	* 
	* @param nTimeout Timeout in nanoseconds (UINT64_MAX = infinite)
	* @param signalSemaphore Semaphore for when the image is available
	* @param signalFence Fence for when the image is available
	* 
	* @returns Acquired image index or UINT32_MAX if failed
	*/
	virtual uint32_t AcquireNextImage(
		uint64_t nTimeout,
		Ref<Semaphore> signalSemaphore,
		Ref<Fence> signalFence
	) = 0;

	/**
	* Presents the actual image in screen
	* 
	* @param nImageIndex Image index
	* @param pWaitSemaphores Semaphore vector to wait before presenting (optional)
	* 
	* @returns True if presentation was successful
	*/
	virtual bool Present(
		uint32_t nImageIndex,
		const Vector<Ref<Semaphore>>& pWaitSemaphores = {}
	) = 0;
	/**
	* Reconstructs the swap chain
	* 
	* @param nNewWidth New width
	* @param nNewHeight New height
	*/
	virtual void Rebuild(uint32_t nNewWidth, uint32_t nNewHeight) = 0;

	/**
	* Gets swap chain's image count
	* 
	* @returns Swap chain image count
	*/
	virtual uint32_t GetImageCount() const = 0;

	/**
	* Gets a swap chain image by index
	* 
	* @param nIndex Image index
	* 
	* @returns The image on that index
	*/
	virtual Ref<GPUTexture> GetImage(uint32_t nIndex) const = 0;
	
	/**
	* Gets a swap chain image view by index
	* 
	* @param nIndex Image index
	* 
	* @returns The image view on that index
	*/
	virtual Ref<ImageView> GetImageView(uint32_t nIndex) const = 0;

	/**
	* Gets depth image
	*
	* @returns Depth image
	*/
	virtual Ref<GPUTexture> GetDepthImage() const = 0;

	/**
	* Gets depth image view if enabled
	*
	* @returns Depth image view
	*/
	virtual Ref<ImageView> GetDepthImageView() const = 0;

	/**
	* Gets depth format
	* 
	* @returns Depth format
	*/
	virtual GPUFormat GetDepthFormat() const = 0;

	/**
	* Gets the actual extent
	* 
	* @returns Extent2D
	*/
	virtual Extent2D GetExtent() const = 0;

	/**
	* Verifies if the swap chain needs to be rebuilt
	* 
	* @returns If needs to be rebuilt
	*/
	virtual bool NeedsRebuild() const = 0;
};