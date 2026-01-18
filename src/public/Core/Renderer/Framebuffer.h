#pragma once
#include "Core/Containers.h"

#include "Core/Renderer/RenderPass.h"
#include "Core/Renderer/ImageView.h"

struct FramebufferCreateInfo {
	Ref<RenderPass> renderPass;
	Vector<Ref<ImageView>> attachments;
	uint32_t nWidth;
	uint32_t nHeight;
};

class Framebuffer {
public:
	static constexpr const char* CLASS_NAME = "Framebuffer";

	using Ptr = Ref<Framebuffer>;

	virtual ~Framebuffer() = default;

	virtual void Create(const FramebufferCreateInfo& createInfo) = 0;
};