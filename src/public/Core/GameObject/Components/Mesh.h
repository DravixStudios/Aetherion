#pragma once
#include "Core/GameObject/Components/Component.h"
#include "Utils.h"
#include "Core/Renderer/ResourceManager.h"

#include "Core/Renderer/MeshData.h"
#include "Core/Resources/AssetHandle.h"

#include "Core/Renderer/Material.h"

#include <map>

class Mesh : public Component {
public:
	Mesh(String name);

	void Start() override;
	void Update() override;

	bool LoadAsset(const AssetHandle& handle);

	MeshData& GetMeshData() { return this->m_meshData; }
	const MeshData& GetMeshData() const { return this->m_meshData; }
	bool IsLoaded() const { return this->m_meshData.bLoaded; }

	void ClearTextureData();

	const AssetHandle 
	GetAssetHandle() { return this->m_meshHandle; }

private:
	Material ProcessMaterial(const MaterialAsset& asset);
	
	MeshData m_meshData;

	AssetHandle m_meshHandle;
};