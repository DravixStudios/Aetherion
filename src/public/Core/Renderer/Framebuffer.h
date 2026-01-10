#pragma once
#include "Core/Containers.h"

#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/GPUTexture.h"

struct FramebufferCreateInfo {
	Ref<RenderPass> renderPass;
	Vector<GPUTexture> attachments;
	uint32_t nWidth;
	uint32_t nHeight;
};

class Framebuffer {
public:
	virtual ~Framebuffer() = default;

	virtual void Create(const FramebufferCreateInfo& createInfo) = 0;
};