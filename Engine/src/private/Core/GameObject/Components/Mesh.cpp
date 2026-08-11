#include "Core/GameObject/Components/Mesh.h"
#include "Core/Core.h"
#include "Core/Renderer/Vulkan/VulkanRenderer.h"

#include "Core/Resources/AssetManager.h"

#include <stb/stb_image.h>

Mesh::Mesh(String name) : Component::Component(name) { }

void 
Mesh::Start() {
	Component::Start();
}

void 
Mesh::Update() {
	Component::Update();
}

/**
* Load mesh asset
* 
* @param handle Mesh asset handle
* 
* @returns True if success
*/
bool 
Mesh::LoadAsset(const AssetHandle& handle) {
	if (this->m_meshData.bLoaded) {
		Logger::Error("Mesh::LoadAsset: Mesh already loaded");
		return false;
	}

	this->m_meshHandle = handle;

	AssetManager* assetMgr = AssetManager::GetInstance();
	const AssetVariant& assetVariant = assetMgr->GetAsset(handle);
	if (assetVariant.valueless_by_exception()) {
		Logger::Error("Mesh::LoadAsset: Invalid asset given");
		return false;
	}


	const MeshAsset& meshAsset = std::get<MeshAsset>(assetVariant);

	uint32_t nSubMeshes = meshAsset.header.nSubMeshCount;
	for (uint32_t i = 0; i < nSubMeshes; i++) {
		const SubMeshAsset& subMesh = meshAsset.subMeshes[i];

		uint32_t nVertexCount = subMesh.header.nVertexCount;
		uint32_t nVertexStride = subMesh.header.nVertexStride;
		uint32_t nVertexOffset = subMesh.header.nVertexOffset;

		uint32_t nIndexCount = subMesh.header.nIndexCount;
		uint32_t nIndexStride = subMesh.header.nIndexStride;
		uint32_t nIndexOffset = subMesh.header.nIndexOffset;

		uint32_t nTotalByteSize = subMesh.header.nTotalByteSize;

		uint32_t nVertexSize = nVertexCount * nVertexStride;
		uint32_t nIndexSize = nIndexCount * nIndexStride;

		/* Check if there's any mismatch with the size */
		if ((nVertexSize + nIndexSize) != nTotalByteSize) {
			Logger::Error("Mesh::LoadAsset: Size mismatch. Expected {} got {}", 
				nTotalByteSize, nVertexSize + nIndexSize
			);

			return false;
		}

		/* Check Vertex size is the same as in vertex stride */
		if (sizeof(Vertex) != nVertexStride) {
			Logger::Error("Mesh::LoadAsset: Stride mismatch. Expected {} got {}", sizeof(Vertex), nVertexStride);
			/* TODO: Handle asset update */
			return false;
		}

		Vector<Vertex> vertices(nVertexCount);
		Vector<uint32_t> indices(nIndexCount);

		const Byte* pVerticesBegin = subMesh.buffer.data();
		const Byte* pIndicesBegin = subMesh.buffer.data() + nVertexSize;

		memcpy(vertices.data(), pVerticesBegin, nVertexSize);
		memcpy(indices.data(), pIndicesBegin, nIndexSize);

		pVerticesBegin = nullptr;
		pIndicesBegin = nullptr;

		AssetHandle materialHandle = subMesh.header.materialHandle;
		Material material;
		if (materialHandle.IsValid()) {
			const AssetVariant& materialVariant = assetMgr->GetAsset(materialHandle);

			if (materialVariant.valueless_by_exception()) {
				Logger::Error("Mesh::LoadAsset: Invalid material");
				continue;
			}

			const MaterialAsset& materialAsset = std::get<MaterialAsset>(materialVariant);

			material = this->ProcessMaterial(materialAsset);
		}

		/* Get material asset handles */
		std::function<TextureData(const AssetHandle&)> loadTexture = 
		[&](const AssetHandle& handle) -> TextureData {
			const AssetVariant& variant = assetMgr->GetAsset(handle);

			if (variant.valueless_by_exception() || !handle.IsValid()) {
				return TextureData{};
			}

			const TextureAsset& asset = std::get<TextureAsset>(variant);
			
			Vector<Byte> texBuffer = asset.buffer;

			TextureData texData = { };
			texData.nWidth = asset.header.nWidth;
			texData.nHeight = asset.header.nHeight;
			texData.name = asset.header.displayName;
			texData.bCompressed = asset.header.bCompressed;
			texData.data = std::move(texBuffer);
			
			return texData;
		};
		
		AssetHandle albedoHandle = material.m_albedoHandle;
		AssetHandle ormHandle = material.m_ormHandle;
		AssetHandle emissiveHandle = material.m_emissiveHandle;
		AssetHandle normalHandle = material.m_normalHandle;

		TextureData albedoData = loadTexture(albedoHandle);
		TextureData ormData = loadTexture(ormHandle);
		TextureData emissiveData = loadTexture(emissiveHandle);
		TextureData normalData = loadTexture(normalHandle);
		
		
		/* Prepare SubMesh data */
		SubMeshData subData = { };
		subData.vertices = std::move(vertices);
		subData.indices = std::move(indices);

		subData.materialFlags = material.GetFlags();
		subData.albedoColor = material.m_albedo;
		subData.ao = material.m_ao;
		subData.roughness = material.m_roughness;
		subData.metallic = material.m_metallic;
		subData.emissiveColor = material.m_emissiveColor;

		subData.albedo = albedoData;
		subData.orm = ormData;
		subData.emissive = emissiveData;
		subData.normal = normalData;

		this->m_meshData.subMeshes[i] = std::move(subData);
	}

	this->m_meshData.name = meshAsset.header.displayName;

	this->m_meshData.bLoaded = true;

	return true;
}

/**
* Clear mesh texture data
*/
void 
Mesh::ClearTextureData() {
	for (auto& [idx, sub] : this->m_meshData.subMeshes) {
		sub.albedo.data.clear();
		sub.albedo.data.shrink_to_fit();
		sub.orm.data.clear();
		sub.orm.data.shrink_to_fit();
		sub.emissive.data.clear();
		sub.emissive.data.shrink_to_fit();
		sub.normal.data.clear();
		sub.normal.data.shrink_to_fit();
	}
}

/**
* Material processor (MaterialAsset -> Material)
* 
* @param asset Material asset
* 
* @returns Processed material
*/
Material 
Mesh::ProcessMaterial(const MaterialAsset& asset) {
	Material material;
	material.Create(asset);

	return material;
}