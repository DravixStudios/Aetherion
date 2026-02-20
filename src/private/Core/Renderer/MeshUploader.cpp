#include "Core/Renderer/MeshUploader.h"
#include <stb/stb_image.h>
#include <xxhash.h>

String HashToString(uint64_t hash) {
	std::ostringstream oss;
	oss << std::hex << std::setw(16) << std::setfill('0') << hash;
	return oss.str();
}

void
MeshUploader::Init(
	Ref<Device> device,
	MegaBuffer* megaBuffer,
	Ref<DescriptorSet> bindlessSet,
	Ref<Sampler> defaultSampler
) {
	this->m_device = device;
	this->m_megaBuffer = megaBuffer;
	this->m_bindlessSet = bindlessSet;
	this->m_defaultSampler = defaultSampler;
	this->m_resourceMgr = ResourceManager::GetInstance();
}

UploadedMesh
MeshUploader::Upload(const MeshData& meshData) {
	UploadedMesh result = { };

	for (auto& [idx, subData] : meshData.subMeshes) {
		UploadedSubMesh uploaded = { };

		uploaded.geometry = this->m_megaBuffer->Upload(subData.vertices, subData.indices);
		uploaded.nAlbedoIndex = this->UploadTexture(subData.albedo);
		uploaded.nORMIndex = this->UploadTexture(subData.orm);
		uploaded.nEmissiveIndex = this->UploadTexture(subData.emissive);
		uploaded.nBlockIdx = uploaded.geometry.nBlockIndex;

		result.subMeshes[idx] = uploaded;
	}

	this->m_bindlessSet->UpdateWrites();

	return result;
}

uint32_t
MeshUploader::UploadTexture(const TextureData& textureData) {
	if (textureData.name.empty() || textureData.data.empty()) {
		return UINT32_MAX;
	}

	if (this->m_resourceMgr->IsTextureRegistered(textureData.name)) {
		return this->m_resourceMgr->GetTextureIndex(textureData.name);
	}

	int nWidth = textureData.nWidth;
	int nHeight = textureData.nHeight;
	unsigned char* pixels = nullptr;
	bool bNeedsFree = false;

	if (textureData.bCompressed) {
		int nChannels;
		pixels = stbi_load_from_memory(
			textureData.data.data(),
			static_cast<int>(textureData.data.size()),
			&nWidth, &nHeight, &nChannels, 4
		);

		if (!pixels) {
			Logger::Error("MeshUploader::UploadTexture: Failed to decompress: {}", textureData.name);
			return UINT32_MAX;
		}

		bNeedsFree = true;
	}
	else {
		pixels = const_cast<unsigned char*>(textureData.data.data());
	}

	size_t size = nWidth * nHeight * 4;
	uint64_t hash = XXH64(pixels, size, 0);
	String hashString = HashToString(hash);
	
	if (this->m_resourceMgr->IsTextureRegistered(hashString)) {
		uint32_t nIdx = this->m_resourceMgr->GetTextureIndex(hashString);

		if (bNeedsFree) {
			stbi_image_free(pixels);
		}

		return nIdx;
	}

	BufferCreateInfo stagingInfo = { };
	stagingInfo.nSize = size;
	stagingInfo.pcData = pixels;
	stagingInfo.sharingMode = ESharingMode::EXCLUSIVE;
	stagingInfo.type = EBufferType::STAGING_BUFFER;
	stagingInfo.usage = EBufferUsage::TRANSFER_SRC;
	
	Ref<GPUBuffer> staging = this->m_device->CreateBuffer(stagingInfo);

	TextureCreateInfo textureInfo = { };
	textureInfo.extent.width = nWidth;
	textureInfo.extent.height = nHeight;
	textureInfo.extent.depth = 1;
	textureInfo.format = GPUFormat::RGBA8_UNORM;
	textureInfo.imageType = ETextureDimensions::TYPE_2D;
	textureInfo.initialLayout = ETextureLayout::UNDEFINED;
	textureInfo.buffer = staging;
	textureInfo.samples = ESampleCount::SAMPLE_1;
	textureInfo.tiling = ETextureTiling::OPTIMAL;
	textureInfo.usage = ETextureUsage::SAMPLED | ETextureUsage::TRANSFER_DST;
	textureInfo.nArrayLayers = 1;
	textureInfo.nMipLevels = 1;
	
	Ref<GPUTexture> texture = this->m_device->CreateTexture(textureInfo);

	ImageViewCreateInfo viewInfo = { };
	viewInfo.image = texture;
	viewInfo.format = GPUFormat::RGBA8_UNORM;
	viewInfo.viewType = EImageViewType::TYPE_2D;
	
	Ref<ImageView> view = this->m_device->CreateImageView(viewInfo);

	uint32_t nTextureIndex = this->m_nNextTextureIndex++;

	DescriptorImageInfo imgInfo = { };
	imgInfo.texture = texture;
	imgInfo.imageView = view;
	imgInfo.sampler = this->m_defaultSampler;

	this->m_bindlessSet->WriteTexture(0, nTextureIndex, imgInfo);

	this->m_resourceMgr->RegisterTexture(hashString, nTextureIndex);

	this->m_textures.push_back(texture);
	this->m_imageViews.push_back(view);

	if (bNeedsFree) {
		stbi_image_free(pixels);
	}

	return nTextureIndex;
}