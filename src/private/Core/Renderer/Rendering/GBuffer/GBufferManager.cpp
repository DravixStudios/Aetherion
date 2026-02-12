#include "Core/Renderer/Rendering/GBuffer/GBufferManager.h"

/**
* G-Buffer manager initialization
* 
* @param device Logical device
* @param nWidth Width
* @param nHeight Height
*/
void
GBufferManager::Init(Ref<Device> device, uint32_t nWidth, uint32_t nHeight) {
	this->m_device = device;
	this->m_nWidth = nWidth;
	this->m_nHeight = nHeight;

	this->CreateTextures();
	this->CreateDescriptors();
}

/**
* Resizes G-Buffer dimensions
*/
void
GBufferManager::Resize(uint32_t nWidth, uint32_t nHeight) {
	if (this->m_nWidth == nWidth && this->m_nHeight == nHeight) return;

	this->m_nWidth = nWidth;
	this->m_nHeight = nHeight;

	this->CreateTextures();
	this->CreateDescriptors();
}

/**
* Creates G-Buffer resources
*/
void
GBufferManager::CreateTextures() {
	/* Create texture lambda */
	std::function<Ref<GPUTexture>(GPUFormat, ETextureUsage)> createTexture = 
		[this](
			GPUFormat format,
			ETextureUsage usage
		) {
			TextureCreateInfo info = { };
			info.imageType = ETextureDimensions::TYPE_2D;
			info.format = format;
			info.extent = { this->m_nWidth, this->m_nHeight, 1 };
			info.nMipLevels = 1;
			info.nArrayLayers = 1;
			info.samples = ESampleCount::SAMPLE_1;
			info.tiling = ETextureTiling::OPTIMAL;
			info.usage = usage;

			return this->m_device->CreateTexture(info);
		};

	/* Create image view lambda */
	std::function<Ref<ImageView>(Ref<GPUTexture>, GPUFormat, bool)> createView =
		[this](
			Ref<GPUTexture> texture,
			GPUFormat format,
			bool bDepth
		) {
			ImageViewCreateInfo info = { };
			info.image = texture;
			info.viewType = EImageViewType::TYPE_2D;
			info.format = format;
			info.subresourceRange.aspectMask = bDepth ? EImageAspect::DEPTH : EImageAspect::COLOR;
			info.subresourceRange.nLevelCount = 1;
			info.subresourceRange.nBaseMipLevel = 0;
			info.subresourceRange.nLayerCount = 1;
			info.subresourceRange.nBaseArrayLayer = 0;

			return this->m_device->CreateImageView(info);
		};

	ETextureUsage colorUsage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
	ETextureUsage depthUsage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;

	this->m_albedo = createTexture(GBufferLayout::ALBEDO, colorUsage);
	this->m_normal = createTexture(GBufferLayout::NORMAL, colorUsage);
	this->m_orm = createTexture(GBufferLayout::ORM, colorUsage);
	this->m_emissive = createTexture(GBufferLayout::EMISSIVE, colorUsage);
	this->m_position = createTexture(GBufferLayout::POSITION, colorUsage);
	this->m_bentNormal = createTexture(GBufferLayout::BENT_NORMAL, colorUsage);
	this->m_depth = createTexture(GBufferLayout::DEPTH, depthUsage);

	this->m_albedoView = createView(this->m_albedo, GBufferLayout::ALBEDO, false);
	this->m_normalView = createView(this->m_normal, GBufferLayout::NORMAL, false);
	this->m_ormView = createView(this->m_orm, GBufferLayout::ORM, false);
	this->m_emissiveView = createView(this->m_emissive, GBufferLayout::EMISSIVE, false);
	this->m_positionView = createView(this->m_position, GBufferLayout::POSITION, false);
	this->m_bentNormalView = createView(this->m_bentNormal, GBufferLayout::BENT_NORMAL, false);
	this->m_depthView = createView(this->m_depth, GBufferLayout::DEPTH, true);

	Logger::Debug("GBufferManager::CreateTextures: G-Buffer Resources created");
}

/**
* Creates G-Buffer descriptors
*/
void 
GBufferManager::CreateDescriptors() {
	if (this->m_readSet) {
		this->m_readSet = Ref<DescriptorSet>();
		this->m_pool = Ref<DescriptorPool>();
		this->m_readLayout = Ref<DescriptorSetLayout>();
		this->m_sampler = Ref<Sampler>();
	}

	/* Create sampler */
	SamplerCreateInfo samplerInfo = { };
	samplerInfo.magFilter = EFilter::LINEAR;
	samplerInfo.minFilter = EFilter::LINEAR;
	samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeV = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.addressModeW = EAddressMode::CLAMP_TO_EDGE;
	samplerInfo.bAnisotropyEnable = false;

	this->m_sampler = this->m_device->CreateSampler(samplerInfo);

	/* Create descriptor set layout */
	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = {
		{ 0, EDescriptorType::COMBINED_IMAGE_SAMPLER, 6, EShaderStage::FRAGMENT },
	};
	
	this->m_readLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Create descriptor pool */
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = 1;
	poolInfo.poolSizes = { { EDescriptorType::COMBINED_IMAGE_SAMPLER, 6 } };

	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Create descriptor set */
	this->m_readSet = this->m_device->CreateDescriptorSet(this->m_pool, this->m_readLayout);

	/* Write descriptors */
	/* Write G-Buffer textures (binding 0, array 0..4) */
	Vector<DescriptorImageInfo> gbufferInfos = {
		{ this->m_albedo, this->m_albedoView, this->m_sampler },
		{ this->m_normal, this->m_normalView, this->m_sampler },
		{ this->m_orm, this->m_ormView, this->m_sampler },
		{ this->m_emissive, this->m_emissiveView, this->m_sampler },
		{ this->m_position, this->m_positionView, this->m_sampler },
		{ this->m_bentNormal, this->m_bentNormalView, this->m_sampler }
	};

	this->m_readSet->WriteTextures(0, 0, gbufferInfos);

	this->m_readSet->UpdateWrites();
}