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