#pragma once
#include "Core/GameObject/Components/Component.h"
#include "Utils.h"
#include "Core/Renderer/ResourceManager.h"

#include <vector>
#include <map>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

/* Forward declarations */
class Core;

class Mesh : public Component {
public:
	Mesh(std::string name);

	void Start() override;
	void Update() override;

	bool LoadModel(std::string filePath);
	bool HasIndices();

	std::map<uint32_t, GPUBuffer*>& GetVBOs();
	std::map<uint32_t, GPUBuffer*>& GetIBOs();
	std::map<uint32_t, GPUTexture*>& GetTextures();
	std::map<uint32_t, uint32_t>& GetTextureIndices();
	std::map<uint32_t, uint32_t>& GetORMIndices();

	std::map<uint32_t, std::vector<Vertex>> m_vertices;
	std::map<uint32_t, GPUBuffer*> m_VBOs;
	std::map<uint32_t, GPUBuffer*> m_IBOs;
	std::map<uint32_t, uint32_t> m_textureIndices;
	std::map<uint32_t, uint32_t> m_ormIndices;
	
private:
	bool m_bMeshImported;
	bool m_bHasIndices;

	/* 
		Texture map:
			Key: The vertex buffer for the texture.
			Value: The texture (GPUTexture).
	*/
	std::map<uint32_t, GPUTexture*> m_textures;
	std::map<uint32_t, GPUTexture*> m_ormTextures;

	ResourceManager* m_resourceManager;

	Core* m_core;
};