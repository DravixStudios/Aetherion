#pragma once
#include "Core/Renderer/Rendering/ResourceHandle.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"

struct GraphNode;

class RenderGraphBuilder {
public:
	/**
	* Creates a new color output texture
	* 
	* @param desc Texture description
	* @param finalLayout Desired final layout after the pass
	* @param loadOp Load operation for the attachment
	* 
	* @returns Texture handle
	*/
	TextureHandle CreateColorOutput(
		const TextureDesc& desc, 
		EImageLayout finalLayout = EImageLayout::COLOR_ATTACHMENT,
		EAttachmentLoadOp loadOp = EAttachmentLoadOp::CLEAR
	);

	/**
	* Creates a new depth output texture
	* 
	* @param desc Texture description
	* @param finalLayout Desired final layout after the pass
	* @param loadOp Load operation for the attachment
	* 
	* @returns Texture handle
	*/
	TextureHandle CreateDepthOutput(
		const TextureDesc& desc, 
		EImageLayout finalLayout = EImageLayout::DEPTH_STENCIL_ATTACHMENT,
		EAttachmentLoadOp loadOp = EAttachmentLoadOp::CLEAR
	);

	/**
	* Registers a texture to be read by the shader
	* 
	* @param handle Texture handle
	* 
	* @returns The same handle
	*/
	TextureHandle ReadTexture(TextureHandle handle);

	/**
	* Uses an existing texture as a color output
	* 
	* @param handle Texture handle
	* @param finalLayout Desired final layout after the pass
	* @param loadOp Load operation for the attachment
	*/
	void UseColorOutput(
		TextureHandle handle, 
		EImageLayout finalLayout = EImageLayout::COLOR_ATTACHMENT, 
		EAttachmentLoadOp loadOp = EAttachmentLoadOp::CLEAR
	);

	/**
	* Uses an existing texture as a depth output
	* 
	* @param handle Texture handle
	* @param finalLayout Desired final layout after the pass
	* @param loadOp Load operation for the attachment
	*/
	void UseDepthOutput(
		TextureHandle handle, 
		EImageLayout finalLayout = EImageLayout::DEPTH_STENCIL_ATTACHMENT,
		EAttachmentLoadOp loadOp = EAttachmentLoadOp::CLEAR
	);

	/**
	* Sets the pass dimensions
	* 
	* @param nWidth Width
	* @param nHeight Height
	*/
	void SetDimensions(uint32_t nWidth, uint32_t nHeight);

	/**
	* Marks this pass as compute-only (no render pass/framebuffer)
	*/
	void SetComputeOnly();
	
private:
	friend class RenderGraph;
	GraphNode* m_node = nullptr;
	TransientResourcePool* m_pool;
};