#pragma once
#include "Core/Containers.h"

#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUFormat.h"
#include "Core/Renderer/Rect2D.h"

enum class EAttachmentLoadOp {
	LOAD,
	CLEAR,
	DONT_CARE
};

enum class EAttachmentStoreOp {
	STORE,
	DONT_CARE
};

enum class EImageLayout {
	UNDEFINED,
	GENERAL,
	COLOR_ATTACHMENT,
	DEPTH_STENCIL_ATTACHMENT,
	DEPTH_STENCIL_READ_ONLY,
	SHADER_READ_ONLY,
	TRANSFER_SRC,
	TRANSFER_DST,
	PRESENT_SRC
};

struct AttachmentDescription {
	GPUFormat format;
	ESampleCount sampleCount = ESampleCount::SAMPLE_1;
	EAttachmentLoadOp loadOp = EAttachmentLoadOp::CLEAR;
	EAttachmentStoreOp storeOp = EAttachmentStoreOp::STORE;
	EAttachmentLoadOp stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	EAttachmentStoreOp stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
	EImageLayout initialLayout = EImageLayout::UNDEFINED;
	EImageLayout finalLayout = EImageLayout::COLOR_ATTACHMENT;
};

struct AttachmentReference {
	uint32_t nAttachment;
	EImageLayout layout;
};

#include "Core/Renderer/PipelineStage.h"

struct SubpassDescription {
	Vector<AttachmentReference> colorAttachments;
	Vector<AttachmentReference> resolveAttachments; // For MSAA
	AttachmentReference depthStencilAttachment;
	AttachmentReference depthResolveAttachment;
	bool bHasDepthStencil = false;
	bool bHasDepthStencilResolve = false;
	Vector<uint32_t> preserveAttachments;
};

constexpr uint32_t SUBPASS_EXTERNAL = ~0U;

struct SubpassDependency {
	uint32_t nSrcSubpass = SUBPASS_EXTERNAL;
	uint32_t nDstSubpass = 0;

	EPipelineStage srcStageMask = EPipelineStage::BOTTOM_OF_PIPE;
	EPipelineStage dstStageMask = EPipelineStage::TOP_OF_PIPE;

	EAccess srcAccessMask = EAccess::NONE;
	EAccess dstAccessMask = EAccess::NONE;
};

struct RenderPassCreateInfo {
	Vector<AttachmentDescription> attachments;
	Vector<SubpassDescription> subpasses;
	Vector<SubpassDependency> dependencies;
};

struct ClearColor {
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 1.f;
};

struct ClearDepthStencil {
	float depth = 1.f;
	uint8_t stencil = 0;
};

struct ClearValue {
	enum class Type { COLOR, DEPTH_STENCIL } type = Type::COLOR;

	union {
		ClearColor color;
		ClearDepthStencil depthStencil;
	};

	ClearValue() : color() {}
	explicit ClearValue(const ClearColor& c) : type(Type::COLOR), color(c) {}
	explicit ClearValue(const ClearDepthStencil& ds) : type(Type::DEPTH_STENCIL), depthStencil(ds) {}
};

class Framebuffer;
class RenderPass;

struct RenderPassBeginInfo {
	Ref<RenderPass> renderPass;
	Ref<Framebuffer> framebuffer;
	Rect2D renderArea;
	Vector<ClearValue> clearValues;
};

class RenderPass {
public:
	static constexpr const char* CLASS_NAME = "RenderPass";

	using Ptr = Ref<RenderPass>;

	virtual ~RenderPass() = default;

	virtual void 
	Create(const RenderPassCreateInfo& createInfo) {
		this->m_createInfo = createInfo;
	}

	const RenderPassCreateInfo& GetCreateInfo() const { return this->m_createInfo; }

protected:
	RenderPassCreateInfo m_createInfo;
};