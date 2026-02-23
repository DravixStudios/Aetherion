#pragma once
#include "Core/GameObject/Components/Component.h"
#include "Utils.h"
#include "Core/Renderer/ResourceManager.h"
#include "Core/Renderer/MeshData.h"

#include <map>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

class Mesh : public Component {
public:
	Mesh(String name);

	void Start() override;
	void Update() override;

	bool LoadModel(String filePath);

	MeshData& GetMeshData() { return this->m_meshData; }
	const MeshData& GetMeshData() const { return this->m_meshData; }
	bool IsLoaded() const { return this->m_meshData.bLoaded; }

	void ClearTextureData();

private:
	MeshData m_meshData;
};