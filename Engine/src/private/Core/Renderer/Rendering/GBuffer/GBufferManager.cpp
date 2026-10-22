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

	this->m_isMultiSampled = this->m_samples != ESampleCount::SAMPLE_1;

	this->CreateTextures();
	this->CreateResolveTargets();
	this->CreateDescriptors();
	this->CreateResolveDescriptors();
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

Ref<GPUTexture>
GBufferManager::CreateTexture(GPUFormat format, ETextureUsage usage, ESampleCount samples)
{
	TextureCreateInfo info = { };
	info.imageType = ETextureDimensions::TYPE_2D;
	info.format = format;
	info.extent = { this->m_nWidth, this->m_nHeight, 1 };
	info.nMipLevels = 1;
	info.nArrayLayers = 1;
	info.samples = this->m_samples;
	info.tiling = ETextureTiling::OPTIMAL;
	info.usage = usage;

	return this->m_device->CreateTexture(info);
}

Ref<ImageView>
GBufferManager::CreateImageView(Ref<GPUTexture> texture, GPUFormat format, bool bIsDepth) {
	ImageViewCreateInfo info = { };
	info.image = texture;
	info.viewType = EImageViewType::TYPE_2D;
	info.format = format;
	info.subresourceRange.aspectMask = bIsDepth ? EImageAspect::DEPTH : EImageAspect::COLOR;
	info.subresourceRange.nLevelCount = 1;
	info.subresourceRange.nBaseMipLevel = 0;
	info.subresourceRange.nLayerCount = 1;
	info.subresourceRange.nBaseArrayLayer = 0;

	return this->m_device->CreateImageView(info);
}

/**
* Creates G-Buffer resources
*/
void
GBufferManager::CreateTextures() {
	const ETextureUsage colorUsage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
	const ETextureUsage depthUsage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;

	this->m_albedo = this->CreateTexture(GBufferLayout::ALBEDO, colorUsage, this->m_samples);
	this->m_normal = this->CreateTexture(GBufferLayout::NORMAL, colorUsage, this->m_samples);
	this->m_orm = this->CreateTexture(GBufferLayout::ORM, colorUsage, this->m_samples);
	this->m_emissive = this->CreateTexture(GBufferLayout::EMISSIVE, colorUsage, this->m_samples);
	this->m_bentNormal = this->CreateTexture(GBufferLayout::BENT_NORMAL, colorUsage, this->m_samples);
	this->m_depth = this->CreateTexture(GBufferLayout::DEPTH, depthUsage, this->m_samples);

	this->m_albedoView = this->CreateImageView(this->m_albedo, GBufferLayout::ALBEDO);
	this->m_normalView = this->CreateImageView(this->m_normal, GBufferLayout::NORMAL);
	this->m_ormView = this->CreateImageView(this->m_orm, GBufferLayout::ORM);
	this->m_emissiveView = this->CreateImageView(this->m_emissive, GBufferLayout::EMISSIVE);
	this->m_bentNormalView = this->CreateImageView(this->m_bentNormal, GBufferLayout::BENT_NORMAL);
	this->m_depthView = this->CreateImageView(this->m_depth, GBufferLayout::DEPTH, true);
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
	const EDescriptorType descriptorType = this->m_isMultiSampled
											? EDescriptorType::STORAGE_IMAGE
											: EDescriptorType::COMBINED_IMAGE_SAMPLER;

	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = {
		{ 0, descriptorType, 6, EShaderStage::FRAGMENT },
	};
	
	this->m_readLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Create descriptor pool */
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = 1;
	poolInfo.poolSizes = { { descriptorType, 6 } };

	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Create descriptor set */
	this->m_readSet = this->m_device->CreateDescriptorSet(this->m_pool, this->m_readLayout);

	Ref<Sampler> sampler = this->m_sampler;
	if (this->m_isMultiSampled) {
		sampler = nullptr;
	}

	/* Write descriptors */
	/* Write G-Buffer textures (binding 0, array 0..4) */
	Vector<DescriptorImageInfo> gbufferInfos = {
		{ this->m_albedo, this->m_albedoView, this->m_sampler, descriptorType },
		{ this->m_normal, this->m_normalView, this->m_sampler, descriptorType },
		{ this->m_orm, this->m_ormView, this->m_sampler, descriptorType },
		{ this->m_emissive, this->m_emissiveView, this->m_sampler, descriptorType },
		{ this->m_depth, this->m_depthView, this->m_sampler, descriptorType },
		{ this->m_bentNormal, this->m_bentNormalView, this->m_sampler, descriptorType }
	};

	this->m_readSet->WriteTextures(0, 0, gbufferInfos);
	this->m_readSet->UpdateWrites();
}

void
GBufferManager::CreateResolveTargets() {
	if (!m_isMultiSampled) {
		return;
	}

	const ETextureUsage colorUsage = ETextureUsage::COLOR_ATTACHMENT | ETextureUsage::SAMPLED;
	const ETextureUsage depthUsage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;

	this->m_albedoResolve = this->CreateTexture(GBufferLayout::ALBEDO, colorUsage);
	this->m_normalResolve = this->CreateTexture(GBufferLayout::NORMAL, colorUsage);
	this->m_ormResolve = this->CreateTexture(GBufferLayout::ORM, colorUsage);
	this->m_emissiveResolve = this->CreateTexture(GBufferLayout::EMISSIVE, colorUsage);
	this->m_bentNormalResolve = this->CreateTexture(GBufferLayout::BENT_NORMAL, colorUsage);
	this->m_depthResolve = this->CreateTexture(GBufferLayout::DEPTH, depthUsage);

	this->m_albedoResolveView = this->CreateImageView(this->m_albedo, GBufferLayout::ALBEDO);
	this->m_normalResolveView = this->CreateImageView(this->m_normal, GBufferLayout::NORMAL);
	this->m_ormResolveView = this->CreateImageView(this->m_orm, GBufferLayout::ORM);
	this->m_emissiveResolveView = this->CreateImageView(this->m_emissive, GBufferLayout::EMISSIVE);
	this->m_bentNormalResolveView = this->CreateImageView(this->m_bentNormal, GBufferLayout::BENT_NORMAL);
	this->m_depthResolveView = this->CreateImageView(this->m_depth, GBufferLayout::DEPTH, true);
}

void
GBufferManager::CreateResolveDescriptors() {
	if (!this->m_isMultiSampled) {
		return;
	}

	if (this->m_resolveReadSet) {
		this->m_resolveReadSet = Ref<DescriptorSet>();
		this->m_resolvePool = Ref<DescriptorPool>();
		this->m_resolveReadLayout = Ref<DescriptorSetLayout>();
		this->m_sampler = Ref<Sampler>();
	}

	/* Create descriptor set layout */
	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = {
			{ 0, EDescriptorType::COMBINED_IMAGE_SAMPLER /* Resolve targets can be combined with a sampler */,
				6,
				EShaderStage::FRAGMENT
			},
		};

	this->m_resolveReadLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Create descriptor pool */
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = 1;
	poolInfo.poolSizes = { { EDescriptorType::COMBINED_IMAGE_SAMPLER, 6 } };

	this->m_resolvePool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Create descriptor set */
	this->m_resolveReadSet = this->m_device->CreateDescriptorSet(this->m_resolvePool, this->m_resolveReadLayout);

	/* Write descriptors */
	/* Write G-Buffer textures (binding 0, array 0..4) */
	Vector<DescriptorImageInfo> gbufferInfos = {
		{ this->m_albedoResolve, this->m_albedoResolveView, this->m_sampler },
		{ this->m_normalResolve, this->m_normalResolveView, this->m_sampler },
		{ this->m_ormResolve, this->m_ormResolveView, this->m_sampler },
		{ this->m_emissiveResolve, this->m_emissiveResolveView, this->m_sampler },
		{ this->m_depthResolve, this->m_depthResolveView, this->m_sampler },
		{ this->m_bentNormalResolve, this->m_bentNormalResolveView, this->m_sampler }
	};

	this->m_resolveReadSet->WriteTextures(0, 0, gbufferInfos);

	this->m_resolveReadSet->UpdateWrites();
}