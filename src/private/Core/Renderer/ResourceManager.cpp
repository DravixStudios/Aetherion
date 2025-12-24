#include "Core/Renderer/ResourceManager.h"
#include "Core/Renderer/Renderer.h"

#include <stb/stb_image.h>

ResourceManager* ResourceManager::m_instance;

ResourceManager::ResourceManager() {
	this->m_renderer = nullptr;
}

GPUTexture* ResourceManager::GetTexture(String textureName) {
	if (!this->TextureExists(textureName)) {
		spdlog::error("ResourceManager::GetTexture: Texture with name {0} not found.", textureName);
		return nullptr;
	}

	return this->m_textures[textureName];
}

bool ResourceManager::AddTexture(String textureName, GPUTexture* pTexture) {
	if (this->TextureExists(textureName)) {
		spdlog::error("ResourceManager::AddTexture: Couldn't add texture {0}. Already exists", textureName);
		return false;
	}

	this->m_textures[textureName] = pTexture;
}

bool ResourceManager::TextureExists(String textureName) {
	if (this->m_textures.count(textureName) > 0)
		return true;
	
	return false;
}

GPUTexture* ResourceManager::UploadTexture(String textureName, const unsigned char* pData, int nWidth, int nHeight) {
	if (this->TextureExists(textureName)) {
		spdlog::error("ResourceManager::UploadTexture: Couldn't add texture {0}. Already exists", textureName);
		return this->m_textures[textureName];
	}
	unsigned char* pixelData = nullptr;

	if (nHeight == 0) {
		int nChannels = 0;

		/* Load image from memory with STB_image */
		pixelData = stbi_load_from_memory(
			pData, // Buffer
			nWidth,  // Size of the file
			&nWidth, // Pointer to width
			&nHeight, // Pointer to height
			&nChannels, // Pointer to channels
			4 // Desired channels
		);

		if (pixelData == nullptr) {
			spdlog::error("ResourceManager::UploadTexture: Failed loading texture from memory");
			throw std::runtime_error("ResourceManager::UploadTexture: Failed loading texture from memory");
			return nullptr;
		}

		GPUBuffer* stagingBuffer = this->m_renderer->CreateStagingBuffer(pixelData, (nWidth * nHeight) * 4);
		GPUTexture* gpuTexture = this->m_renderer->CreateTexture(stagingBuffer, nWidth, nHeight, GPUFormat::RGBA8_SRGB);

		/* Free image data */
		stbi_image_free(pixelData);
		pixelData = nullptr;

		/* Delete staging buffer */
		delete stagingBuffer;
		stagingBuffer = nullptr;

		this->AddTexture(textureName, gpuTexture);

		return gpuTexture;
	}
	else {
		/* TODO: Load uncompressed textures */
		return nullptr;
	}
}

void ResourceManager::SetRenderer(Renderer* pRenderer) {
	this->m_renderer = pRenderer;
}

ResourceManager* ResourceManager::GetInstance() {
	if (ResourceManager::m_instance == nullptr) {
		ResourceManager::m_instance = new ResourceManager();
	}

	return ResourceManager::m_instance;
}