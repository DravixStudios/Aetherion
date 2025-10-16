#pragma once
#include <iostream>
#include <map>

#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"

class ResourceManager {
public:
	ResourceManager();

	GPUTexture* GetTexture(std::string textureName);
	bool TextureExists(std::string textureName);

	static ResourceManager* GetInstance();
private:
	static ResourceManager* m_instance;
};