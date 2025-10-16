#pragma once
#include <iostream>
#include <map>

#include <spdlog/spdlog.h>

#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"

class ResourceManager {
public:
	ResourceManager();

	GPUTexture* GetTexture(std::string textureName);
	bool AddTexture(std::string textureName, GPUTexture* pTexture);
	bool TextureExists(std::string textureName);

	static ResourceManager* GetInstance();
private:
	std::map<std::string, GPUTexture*> m_textures;

	static ResourceManager* m_instance;
};