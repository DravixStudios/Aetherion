#pragma once
#include <iostream>
#include <map>

#include <spdlog/spdlog.h>

#include "Core/Containers.h" 
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"

class Renderer;

class ResourceManager {
public:
	ResourceManager();

	GPUTexture* GetTexture(String textureName);
	bool AddTexture(String textureName, GPUTexture* pTexture);
	bool TextureExists(String textureName);

	GPUTexture* UploadTexture(String textureName, const unsigned char* pData, int nWidth, int nHeight);

	void SetRenderer(Renderer* pRenderer);

	static ResourceManager* GetInstance();
private:
	std::map<String, GPUTexture*> m_textures;
	Renderer* m_renderer;

	static ResourceManager* m_instance;
};