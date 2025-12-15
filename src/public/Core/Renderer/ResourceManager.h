#pragma once
#include <iostream>
#include <map>

#include <spdlog/spdlog.h>

#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"

class Renderer;

class ResourceManager {
public:
	ResourceManager();

	GPUTexture* GetTexture(std::string textureName);
	bool AddTexture(std::string textureName, GPUTexture* pTexture);
	bool TextureExists(std::string textureName);

	GPUTexture* UploadTexture(std::string textureName, const unsigned char* pData, int nWidth, int nHeight);

	void SetRenderer(Renderer* pRenderer);

	static ResourceManager* GetInstance();
private:
	std::map<std::string, GPUTexture*> m_textures;
	Renderer* m_renderer;

	static ResourceManager* m_instance;
};