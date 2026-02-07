#pragma once
#include <map>

#include "Core/Containers.h" 

class ResourceManager {
public:
	ResourceManager() = default;

	bool IsTextureRegistered(const String& name) const;
	void RegisterTexture(const String& name, uint32_t nBindlessIndex);
	uint32_t GetTextureIndex(const String& name) const;

	static ResourceManager* GetInstance();
private:
	static ResourceManager* m_instance;

	std::map<String, uint32_t> m_textureIndices;
};