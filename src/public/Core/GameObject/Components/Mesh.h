#pragma once
#include "Core/GameObject/Components/Component.h"
#include "Utils.h"
#include "Core/Renderer/ResourceManager.h"

#include <map>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

/* Forward declarations */
class Core;
class VulkanRenderer;

class Mesh : public Component {
public:
	struct SubMesh {
		uint32_t nVertexOffset; // Where starts the VBO in the global buffer
		uint32_t nFirstIndex; // Where starts the IBO in the global buffer
		uint32_t nIndexCount; // Index count
	};

	Mesh(String name);
	
	void Start() override;
	void Update() override;

	bool LoadModel(String filePath);
	bool HasIndices();

	std::map<uint32_t, SubMesh>& GetSubMeshes();
	std::map<uint32_t, GPUTexture*>& GetTextures();
	std::map<uint32_t, uint32_t>& GetTextureIndices();
	std::map<uint32_t, uint32_t>& GetORMIndices();
	std::map<uint32_t, uint32_t>& GetEmissiveIndices();

	std::map<uint32_t, SubMesh> m_subMeshes;

	uint32_t RegisterVulkan(VulkanRenderer* pRenderer, String textureName, GPUTexture* gpuTexture);

	std::map<uint32_t, Vector<Vertex>> m_vertices;
	std::map<uint32_t, uint32_t> m_textureIndices;
	std::map<uint32_t, uint32_t> m_ormIndices;
	std::map<uint32_t, uint32_t> m_emissiveIndices;
	
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
	std::map<uint32_t, GPUTexture*> m_emissiveTextures;

	ResourceManager* m_resourceManager;

	Core* m_core;
};