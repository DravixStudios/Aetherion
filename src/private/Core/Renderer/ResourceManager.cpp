#include "Core/Renderer/ResourceManager.h"

ResourceManager* ResourceManager::m_instance;

/**
* Checks if the texture is registered
* 
* @param name Texture name
* 
* @returns True if texture is registered
*/
bool 
ResourceManager::IsTextureRegistered(const String& name) const {
	return this->m_textureIndices.count(name) > 0;
}

/**
* Registers the texture in the
* resource manager
* 
* @param name Texture name
* @param nBindlessIndex Bindless index
*/
void
ResourceManager::RegisterTexture(const String& name, uint32_t nBindlessIndex) {
	this->m_textureIndices[name] = nBindlessIndex;
}

/**
* Get texture index
* 
* @param name Texture name
* 
* @returns Texture index
*/
uint32_t
ResourceManager::GetTextureIndex(const String& name) const {
	std::map<String, uint32_t>::const_iterator it = this->m_textureIndices.find(name);
	return (it != this->m_textureIndices.end()) ? it->second : UINT32_MAX;
}

ResourceManager* 
ResourceManager::GetInstance() {
	if (ResourceManager::m_instance == nullptr) {
		ResourceManager::m_instance = new ResourceManager();
	}

	return ResourceManager::m_instance;
}