#pragma once
#include "Core/Renderer/Rendering/ResourceHandle.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"

struct GraphNode;

class RenderGraphBuilder {
public:
	TextureHandle CreateColorOutput(const TextureDesc& desc);
	TextureHandle CreateDepthOutput(const TextureDesc& desc);
	TextureHandle ReadTexture(TextureHandle handle);

	void UseColorOutput(TextureHandle handle);
	void SetDimensions(uint32_t nWidth, uint32_t nHeight);
	
private:
	friend class RenderGraph;
	GraphNode* m_node = nullptr;
	TransientResourcePool* m_pool;
};