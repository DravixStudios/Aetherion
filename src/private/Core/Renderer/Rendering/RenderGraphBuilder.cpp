#include "Core/Renderer/Rendering/RenderGraphBuilder.h"
#include "Core/Renderer/Rendering/GraphNode.h"

/**
 * Creates a new color output
 * 
 * @param desc Texture desc
 * 
 * @returns Texture handle
 */
TextureHandle
RenderGraphBuilder::CreateColorOutput(
    const TextureDesc& desc, 
    EImageLayout finalLayout, 
    EAttachmentLoadOp loadOp
) {
    TextureHandle handle = this->m_pool->AcquireTexture(desc);
    this->m_node->colorOutputs.push_back(handle);
    this->m_node->colorFinalLayouts.push_back(finalLayout);
    this->m_node->colorLoadOps.push_back(loadOp);
    this->m_node->nWidth = desc.nWidth;
    this->m_node->nHeight = desc.nHeight;

    return handle;
}

/**
 * Creates a new depth output
 * 
 * @param desc Texture desc
 * 
 * @returns Depth texture handle
 */
TextureHandle
RenderGraphBuilder::CreateDepthOutput(
    const TextureDesc& desc, 
    EImageLayout finalLayout, 
    EAttachmentLoadOp loadOp
) {
    TextureHandle handle = this->m_pool->AcquireTexture(desc);
    this->m_node->depthOutput = handle;
    this->m_node->depthFinalLayout = finalLayout;
    this->m_node->depthLoadOp = loadOp;
    this->m_node->bHasDepth = true;
    this->m_node->nWidth = desc.nWidth;
    this->m_node->nHeight = desc.nHeight;

    return handle;
}

/**
 * Reads a texture
 * 
 * @param handle Texture handle
 * 
 * @returns Same texture handle
 */
TextureHandle
RenderGraphBuilder::ReadTexture(TextureHandle handle) {
    this->m_node->textureInputs.push_back(handle);
    return handle;
}

/**
 * Adds a new color output
 * 
 * @param handle Color output handle
 */
void
RenderGraphBuilder::UseColorOutput(TextureHandle handle, EImageLayout finalLayout, EAttachmentLoadOp loadOp) {
    this->m_node->colorOutputs.push_back(handle);
    this->m_node->colorFinalLayouts.push_back(finalLayout);
    this->m_node->colorLoadOps.push_back(loadOp);
}

/**
 * Adds a depth output
 * 
 * @param handle Depth output handle
 */
void
RenderGraphBuilder::UseDepthOutput(TextureHandle handle, EImageLayout finalLayout, EAttachmentLoadOp loadOp) {
    this->m_node->depthOutput = handle;
    this->m_node->depthFinalLayout = finalLayout;
    this->m_node->bHasDepth = true;
    this->m_node->depthLoadOp = loadOp;
}

/**
 * Sets render graph dimensions
 * 
 * @param nWidth Width
 * @param nHeight Height
 */
void
RenderGraphBuilder::SetDimensions(uint32_t nWidth, uint32_t nHeight) {
    this->m_node->nWidth = nWidth;
    this->m_node->nHeight = nHeight;
}

/**
* Marks the node as compute-only
* (No render pass or framebuffer needed)
*/
void
RenderGraphBuilder::SetComputeOnly() {
    this->m_node->bIsComputeOnly = true;
}