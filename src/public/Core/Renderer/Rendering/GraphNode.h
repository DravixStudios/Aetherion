#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GraphicsContext.h"
#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/Framebuffer.h"
#include "Core/Renderer/Rendering/ResourceHandle.h"

#include <functional>

class RenderGraphContext;

struct GraphNode {
	const char* name = nullptr;

	Vector<TextureHandle> colorOutputs;
	TextureHandle depthOutput;
	bool bHasDepth = false;
	Vector<TextureHandle> textureInputs;

	Ref<RenderPass> renderPass;
	Ref<Framebuffer> framebuffer;

	uint32_t nWidth = 0;
	uint32_t nHeight = 0;

	std::function<void(Ref<GraphicsContext>, RenderGraphContext&)> execute;

	Vector<uint32_t> dependsOn; // Indices of the nodes that depends on
};