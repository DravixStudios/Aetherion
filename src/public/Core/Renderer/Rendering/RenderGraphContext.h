#pragma once
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/Rendering/ResourceHandle.h"

class TransientResourcePool;

class RenderGraphContext {
public:
	Ref<GPUTexture> GetTexutre(TextureHandle handle);
	Ref<ImageView> GetImageView(TextureHandle handle);

private:
	friend class RenderGraph;
	TransientResourcePool* m_pool;
};