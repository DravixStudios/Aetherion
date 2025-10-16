#include "Core/Renderer/ResourceManager.h"

ResourceManager* ResourceManager::m_instance;

ResourceManager::ResourceManager() {

}

GPUTexture* ResourceManager::GetTexture(std::string textureName) {
	if (!this->TextureExists(textureName)) {
		spdlog::error("ResourceManager::GetTexture: Texture with name {0} not found.", textureName);
		return nullptr;
	}

	return this->m_textures[textureName];
}

bool ResourceManager::AddTexture(std::string textureName, GPUTexture* pTexture) {
	if (this->TextureExists(textureName)) {
		spdlog::error("ResourceManager::AddTexture: Couldn't add texture {0}. Already exists", textureName);
		return false;
	}

	this->m_textures[textureName] = pTexture;
}

bool ResourceManager::TextureExists(std::string textureName) {
	if (this->m_textures.count(textureName) > 0)
		return true;
	
	return false;
}

ResourceManager* ResourceManager::GetInstance() {
	if (ResourceManager::m_instance == nullptr) {
		ResourceManager::m_instance = new ResourceManager();
	}

	return ResourceManager::m_instance;
}