#pragma once
#include "Core/Containers.h"

#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUFormat.h"

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
	uint32_t nSampleCount = 1;
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

struct SubpassDescription {
	Vector<AttachmentReference> colorAttachments;
	Vector<AttachmentReference> resolveAttachments; // For MSAA
	AttachmentReference depthStencilAttachment;
	bool bHasDepthStencil = false;
	Vector<uint32_t> preserveAttachments;
};

/* 
	Simplified subpass dependency implementation
	Full implementation will be done during the
	development.

	TODO: Finish subpass dependency implementation
*/
struct SubpassDependency {
	uint32_t nSrcSubpass; // VK_SUBPASS_EXTERNAL = ~0U for external dependencies
	uint32_t nDstSubpass;
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
		ClearDepthStencil deptStencil;
	};

	ClearValue() : color() {}
	explicit ClearValue(const ClearColor& c) : type(Type::COLOR), color(c) {}
	explicit ClearValue(const ClearDepthStencil& ds) : type(Type::DEPTH_STENCIL), deptStencil(ds) {}
};

struct Rect2D {
	int32_t x = 0;
	int32_t y = 0;
	uint32_t width = 0;
	uint32_t height = 0;
};

/* 
	Framebuffer forward declaration
	TODO: Implement framebuffer abstraction
*/
class Framebuffer;

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