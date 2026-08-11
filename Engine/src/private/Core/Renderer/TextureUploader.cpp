#include "Core/Renderer/TextureUploader.h"
#include "Core/Renderer/Device.h"
#include <chrono>
#include <algorithm>

TextureUploader::TextureUploader(Ref<Device> device) : m_device(device) { }

/**
* Initializes the texture uploader
* 
* @param nMaxConcurrentUploads Max concurrent uploads (default = 4)
*/
void
TextureUploader::Init(uint32_t nMaxConcurrentUploads) {
	this->m_nMaxConcurrentUploads = nMaxConcurrentUploads;

	/* Use hardware concurrency or specified limit */
	uint32_t nNumThreads = std::min(
		static_cast<uint32_t>(std::thread::hardware_concurrency()),
		this->m_nMaxConcurrentUploads
	);

	this->m_threadPool = ThreadPool::CreateShared(nNumThreads);
	
	Logger::Info("TextureUploader::Init: Initialized with {} worker threads", nNumThreads);
}

/**
* Queue texture upload job (non-blocking)
*
* @param createInfo Texture create info
* @param pixelData Raw pixel data
* @param debugName Debug name for logging (optional)
*
* @returns Future to retrieve the uploaded texture
*/
std::future<GPUTexture::Ptr>
TextureUploader::QueueUpload(
	const TextureCreateInfo& createInfo, 
	Vector<uint8_t> pixelData, 
	const String& debugName
) {
	/* Submit upload task to thread pool */
	auto future = this->m_threadPool->Submit(
		&TextureUploader::UploadTextureTask,
		this,
		createInfo,
		std::move(pixelData),
		debugName
	);

	return std::move(future);
}

/**
* Internal upload task executed by thread pool
*
* @param createInfo Texture create info
* @param pixelData Raw pixel data
* @param debugName Debug name for logging
*
* @returns Uploaded texture
*/
GPUTexture::Ptr
TextureUploader::UploadTextureTask(
	const TextureCreateInfo& createInfo,
	Vector<uint8_t> pixelData,
	const String& debugName
) {
	auto startTime = std::chrono::high_resolution_clock::now();

	struct ThreadLocalContext {
		Ref<CommandPool> commandPool;
		Ref<UploadContext> uploadContext;
	};

	static thread_local ThreadLocalContext threadContext;

	if (!threadContext.commandPool) {
		CommandPoolCreateInfo poolInfo = { };
		poolInfo.flags = ECommandPoolFlags::RESET_COMMAND_BUFFER;
		
		threadContext.commandPool = this->m_device->CreateCommandPool(poolInfo, EQueueType::GRAPHICS);
		threadContext.uploadContext = CreateRef<UploadContext>();
		threadContext.uploadContext->commandBuffer = threadContext.commandPool->AllocateCommandBuffer();
	}

	try {
		/* Create staging buffer */
		uint32_t nBufferSize = pixelData.size();

		BufferCreateInfo bufferInfo = { };
		bufferInfo.pcData = pixelData.data();
		bufferInfo.nSize = nBufferSize;
		bufferInfo.sharingMode = ESharingMode::EXCLUSIVE;
		bufferInfo.type = EBufferType::STAGING_BUFFER;
		bufferInfo.usage = EBufferUsage::TRANSFER_SRC;
		
		Ref<GPUBuffer> stagingBuffer = this->m_device->CreateBuffer(bufferInfo);

		/* Copy pixel data to staging buffer */
		void* pData = stagingBuffer->Map();
		memcpy(pData, pixelData.data(), nBufferSize);
		stagingBuffer->Unmap();

		threadContext.uploadContext->commandBuffer->Reset();
		threadContext.uploadContext->commandBuffer->Begin(true);

		/* Create a texture with staging buffer */
		TextureCreateInfo textureInfo = createInfo;
		textureInfo.buffer = stagingBuffer;
		textureInfo.uploadContext = threadContext.uploadContext;

		GPUTexture::Ptr texture = this->m_device->CreateTexture(textureInfo);

		threadContext.uploadContext->commandBuffer->End();

		/* Create a fence for the thread */
		FenceCreateInfo fenceInfo = { };
		fenceInfo.flags = EFenceFlags::SIGNALED;

		Ref<Fence> fence = this->m_device->CreateFence(fenceInfo);
		fence->Reset();

		/* Submit to graphics queue */
		Vector<Ref<CommandBuffer>> commandBuffers = { threadContext.uploadContext->commandBuffer };

		SubmitInfo submitInfo = { };
		submitInfo.commandBuffers = commandBuffers;

		this->m_device->Submit(submitInfo, fence);

		this->m_device->WaitForFence(fence);

		stagingBuffer = Ref<GPUBuffer>();

		return texture;
	}
	catch (const std::exception& e) {
		Logger::Error("TextureUploader::UploadTexture: Failed to upload texture {}: {}", debugName, e.what());
		throw;
	}
}