#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Core/Renderer/Rendering/TransientResourcePool.h"

/**
* Gets a texture
* 
* @param handle Texture handle
* 
* @returns Texture
*/
Ref<GPUTexture>
RenderGraphContext::GetTexture(TextureHandle handle) {
    return this->m_pool->GetTexture(handle);
}

/**
* Gets a Image view
* 
* @param handle Texture handle
* 
* @returns Image view
*/
Ref<ImageView>
RenderGraphContext::GetImageView(TextureHandle handle) {
    return this->m_pool->GetImageView(handle);
}