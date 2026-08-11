#pragma once
#include "Core/Renderer/GPUTexture.h"
#include "Core/Utils/ThreadPool.h"

#include <future>
#include <mutex>
#include <functional>

class Device;

struct TextureUploadJob {
	std::future<GPUTexture::Ptr> future;
	String debugName;
};

class TextureUploader {
public:
	using Ptr = Ref<TextureUploader>;

	explicit TextureUploader(Ref<Device> device);
	~TextureUploader() = default;

	void Init(uint32_t nMaxConcurrentUploads = 4);

	std::future<GPUTexture::Ptr> QueueUpload(
		const TextureCreateInfo& createInfo,
		Vector<uint8_t> pixelData,
		const String& debugName = ""
	);

	static Ptr
	CreateShared(Ref<Device> device) {
		return CreateRef<TextureUploader>(device);
	}

private:
	Ref<Device> m_device;
	Ref<ThreadPool> m_threadPool;
	std::mutex m_jobsMutex;
	uint32_t m_nMaxConcurrentUploads;

	GPUTexture::Ptr UploadTextureTask(
		const TextureCreateInfo& createInfo,
		Vector<uint8_t> pixelData,
		const String& debugName
	);
};