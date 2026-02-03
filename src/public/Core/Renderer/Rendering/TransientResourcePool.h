#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/Rendering/ResourceHandle.h"

struct TextureDesc {
	GPUFormat format;
	uint32_t nWidth;
	uint32_t nHeight;
	ETextureUsage usage;
	const char* debugName = nullptr;
};

class TransientResourcePool {
public:
	void Init(Ref<Device> device);

	TextureHandle AcquireTexture(const TextureDesc& desc);
	TextureHandle ImportTexture(Ref<GPUTexture> texture, Ref<ImageView> view);

	Ref<GPUTexture> GetTexture(TextureHandle handle);
	Ref<ImageView> GetImageView(TextureHandle handle);

	void BeginFrame();
	void EndFrame();

private:
	struct Entry {
		TextureDesc desc;
		Ref<GPUTexture> texture;
		Ref<ImageView> view;
		bool bImported = false;
		uint32_t nLastFrame = 0;
	};

	Ref<Device> m_device;
	Vector<Entry> m_entries;
	uint32_t m_nFrame = 0;
};