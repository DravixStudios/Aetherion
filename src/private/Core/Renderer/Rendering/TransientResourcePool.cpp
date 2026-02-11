#include "Core/Renderer/Rendering/TransientResourcePool.h"

/**
* Initializes the Transient resource pool
*/
void
TransientResourcePool::Init(Ref<Device> device) {
    this->m_device = device;
    this->m_nFrame = 0;
}

/**
* Acquires a texutre. 
* If there is any reusable texture, we'll use that,
* instead, we create a one
* 
* @param desc Texture desc
* 
* @returns Texture handle
*/
TextureHandle
TransientResourcePool::AcquireTexture(const TextureDesc& desc) {
    /* Search for a reusable texture */
    for(uint32_t i = 0; i < this->m_entries.size(); ++i) {
        Entry& entry = this->m_entries[i];
        if(!entry.bImported &&
            entry.desc.format == desc.format &&
            entry.desc.nWidth == desc.nWidth &&
            entry.desc.nHeight == desc.nHeight &&
            static_cast<uint32_t>(entry.desc.usage) == static_cast<uint32_t>(desc.usage)&&
            entry.nLastFrame != this->m_nFrame
        ) {
            entry.nLastFrame = this->m_nFrame;
            return { i, 0 };
        }
    }

    /* Create new texture */
    TextureCreateInfo texInfo = { };
    texInfo.imageType = ETextureDimensions::TYPE_2D;
    texInfo.format = desc.format;
    texInfo.extent = { desc.nWidth, desc.nHeight, 1 };
    texInfo.nMipLevels = 1;
    texInfo.nArrayLayers = 1;
    texInfo.samples = ESampleCount::SAMPLE_1;
    texInfo.tiling = ETextureTiling::OPTIMAL;
    texInfo.usage = desc.usage;
    texInfo.initialLayout = ETextureLayout::UNDEFINED;

    Ref<GPUTexture> texture = this->m_device->CreateTexture(texInfo);

    /* Create new image view */
    ImageViewCreateInfo viewInfo = { };
    viewInfo.image = texture;
    viewInfo.viewType = EImageViewType::TYPE_2D;
    viewInfo.format = desc.format;

    bool bDepth = (desc.format == GPUFormat::D32_FLOAT ||
                   desc.format == GPUFormat::D24_UNORM_S8_UINT ||
                   desc.format == GPUFormat::D32_FLOAT_S8_UINT);
    
    viewInfo.subresourceRange.aspectMask = bDepth ? EImageAspect::DEPTH : EImageAspect::COLOR;
    viewInfo.subresourceRange.nBaseMipLevel = 0;
    viewInfo.subresourceRange.nLevelCount = 1;
    viewInfo.subresourceRange.nBaseArrayLayer = 0;
    viewInfo.subresourceRange.nLayerCount = 1;

    Ref<ImageView> view = this->m_device->CreateImageView(viewInfo);

    Entry entry = { };
    entry.desc = desc;
    entry.texture = texture;
    entry.view = view;
    entry.bImported = false;
    entry.nLastFrame = this->m_nFrame;

    uint32_t idx = static_cast<uint32_t>(this->m_entries.size());
    this->m_entries.push_back(std::move(entry));
    return { idx, 0 };
}

/**
* Imports a texture
* 
* @param texture Texture
* @param view Image view
* 
* @returns Texture handle
*/
TextureHandle
TransientResourcePool::ImportTexture(Ref<GPUTexture> texture, Ref<ImageView> view) {
    Entry entry = { };
    entry.texture = texture;
    entry.view = view;
    entry.bImported = true;
    entry.nLastFrame = this->m_nFrame;

    uint32_t idx = static_cast<uint32_t>(this->m_entries.size());
    this->m_entries.push_back(std::move(entry));
    return { idx, 0 };
}

/**
* Gets a texture
* 
* @param handle Texture handle
* 
* @returns Texture
*/
Ref<GPUTexture>
TransientResourcePool::GetTexture(TextureHandle handle) {
    if(!handle.IsValid() || handle.nIndex >= this->m_entries.size()) return { };
    return this->m_entries[handle.nIndex].texture;
}

/**
* Gets a image view
* 
* @param handle Texture handle
* 
* @returns Image view
*/
Ref<ImageView>
TransientResourcePool::GetImageView(TextureHandle handle) {
    if(!handle.IsValid() || handle.nIndex >= this->m_entries.size()) return { };
    return this->m_entries[handle.nIndex].view;
}

/**
* Begins a new frame
*/
void
TransientResourcePool::BeginFrame() {
    /* Clean last frame's imports */
    this->m_entries.erase(
        std::remove_if(
            this->m_entries.begin(),
            this->m_entries.end(),
            [](const Entry& entry) { return entry.bImported; }
        ),
        this->m_entries.end()
    );
}

/**
* Ends the frame
*/
void
TransientResourcePool::EndFrame() {
    ++this->m_nFrame;
}

/**
* Invalidates the resource pool
*/
void
TransientResourcePool::Invalidate() {
	this->m_entries.clear();
}