#include "Core/Resources/AssetManager.h"
#include <fstream>
#include <filesystem>
#include <array>

#include <string>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Core/Renderer/GPUTexture.h"

namespace fs = std::filesystem;

AssetManager* AssetManager::m_instance;

AssetManager::AssetManager() {}

enum class EImportedAssetType : uint32_t {
	MESH = 0x01,
	TEXTURE = 0x02
};

static Map<String, EImportedAssetType> s_extensionTypes = {
	/* Mesh assets */
	{ ".glb", EImportedAssetType::MESH },
	{ ".fbx", EImportedAssetType::MESH },
	{ ".obj", EImportedAssetType::MESH },
	{ ".gltf", EImportedAssetType::MESH },
	{ ".dae", EImportedAssetType::MESH },
	{ ".stl", EImportedAssetType::MESH },

	/* Texture assets */
	{ ".png", EImportedAssetType::TEXTURE },
	{ ".jpg", EImportedAssetType::TEXTURE },
	{ ".exr", EImportedAssetType::TEXTURE },
	{ ".hdr", EImportedAssetType::TEXTURE },
	{ ".tga", EImportedAssetType::TEXTURE },
	{ ".tif", EImportedAssetType::TEXTURE },
	{ ".tiff", EImportedAssetType::TEXTURE },
};

/**
* Writes a MeshAsset into a ".aeth" file
* 
* TODO: Load the mesh to the project folder
* 
* @param filename File name
* @param asset Mesh asset data
* 
* @returns True if success
*/
bool 
AssetManager::SaveMesh(const String& filename, const MeshAsset& asset) {
	/* 
		Get the executable path 
		TODO: Use project directory
	*/
	String exePath = GetExecutableDir();

	std::filesystem::path filePath = std::filesystem::path(exePath) / filename;

	/* Open file */
	std::ofstream file(filePath, std::ios::binary);

	if (!file.is_open()) {
		Logger::Error("AssetManager::SaveMesh: Failed opening file: {}", filename);
		return false;
	}

	EAssetType type = EAssetType::MESH;

	/* Write MeshAssetHeader */
	uint64_t nRawVersion = MESH_VERSION.Serialize();

	file.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
	file.write(reinterpret_cast<const char*>(&nRawVersion), sizeof(uint64_t));
	file.write(reinterpret_cast<const char*>(&type), sizeof(EAssetType));

	static_assert(std::is_trivially_copyable_v<MeshAssetHeader>);
	file.write(reinterpret_cast<const char*>(&asset.header), sizeof(MeshAssetHeader));

	/* Check if file is good */
	if (!file.good()) return false;

	/* Copy MeshAsset data to file */
	if (asset.subMeshes.size() != asset.header.nSubMeshCount) {
		Logger::Error("AssetManager::WriteMesh: SubMesh count mismatch. Expected {} got {}",
			asset.header.nSubMeshCount,
			asset.subMeshes.size()
		);

		return false;
	}

	/* Copy SubMeshes */
	static_assert(std::is_trivially_copyable_v<SubMeshAssetHeader>);

	for (const SubMeshAsset& subMesh : asset.subMeshes) {
		file.write(reinterpret_cast<const char*>(&subMesh.header), sizeof(SubMeshAssetHeader));

		uint32_t nBufferSize = subMesh.buffer.size() * sizeof(Byte);
		
		std::streampos beforeBuffer = file.tellp();
		file.write(reinterpret_cast<const char*>(subMesh.buffer.data()), nBufferSize);
		std::streampos afterBuffer = file.tellp();

		uint32_t nWrittenBytes = static_cast<uint32_t>(afterBuffer - beforeBuffer);

		if (nWrittenBytes != nBufferSize) {
			Logger::Error("AssetManager::SaveMesh: SubMesh buffer size mismatch. Expected {} got {}",
				nBufferSize, nWrittenBytes
			);

			return false;
		}
	}

	file.write(reinterpret_cast<const char*>(
		asset.subMeshes.data()),
		sizeof(SubMeshAsset) * asset.header.nSubMeshCount
	);

	if (!file) return false;

	file.close();

	return true;
}

/**
* Writes a SceneAsset into a .aeth file
* 
* @param filename File name
* @param asset Scene asset
* 
* @returns True if success
*/
bool 
AssetManager::SaveScene(const String& filename, const SceneAsset& asset) {
	/*
		Get the executable path
		TODO: Use project directory
	*/
	String exePath = GetExecutableDir();

	std::filesystem::path filePath = std::filesystem::path(exePath) / filename;

	/* Open file */
	std::ofstream file(filePath, std::ios::binary);

	if (!file.is_open()) {
		Logger::Error("AssetManager::SaveScene: Failed opening file: {}", filename);
		return false;
	}

	EAssetType type = EAssetType::SCENE;

	/* Write SceneAssetHeader */
	uint64_t nRawVersion = SCENE_VERSION.Serialize();

	file.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
	file.write(reinterpret_cast<const char*>(&nRawVersion), sizeof(uint64_t));
	file.write(reinterpret_cast<const char*>(&type), sizeof(EAssetType));

	static_assert(std::is_trivially_copyable_v<SceneAssetHeader>);
	file.write(reinterpret_cast<const char*>(&asset.header), sizeof(SceneAssetHeader));

	/* Check if file is good */
	if (!file.good()) return false;

	/* Write scene objects to file */
	Vector<GameObjectAsset> objects = asset.objects;

	if (objects.size() != asset.header.nObjectCount) {
		Logger::Error(
			"AssetManager::SaveScene: Object count mismatch. Expected {} got {}", 
			asset.header.nObjectCount,
			objects.size()
		);

		return false;
	}

	file.write(reinterpret_cast<const char*>(objects.data()), asset.header.nObjectCount * sizeof(GameObjectAsset));

	if (!file) return false;

	file.close();

	return true;
}

/**
* Writes a material asset into a .aeth file
*
* @param filename File name
* @param asset Material asset data
*
* @returns True if successfully written
*/
bool
AssetManager::SaveMaterial(const String& filename, const MaterialAsset& asset) {
	/*
		Get the executable path
		TODO: Use project directory
	*/
	String exePath = GetExecutableDir();

	std::filesystem::path filePath = std::filesystem::path(exePath) / filename;

	/* Open file */
	std::ofstream file(filePath, std::ios::binary);

	if (!file.is_open()) {
		Logger::Error("AssetManager::SaveTexture: Failed opening file: {}", filename);
		return false;
	}

	EAssetType type = EAssetType::MATERIAL;

	/* Write MaterialAssetHeader */
	uint64_t nRawVersion = MATERIAL_VERSION.Serialize();

	file.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
	file.write(reinterpret_cast<const char*>(&nRawVersion), sizeof(uint64_t));
	file.write(reinterpret_cast<const char*>(&type), sizeof(EAssetType));

	static_assert(std::is_trivially_copyable_v<MaterialAssetHeader>);
	file.write(reinterpret_cast<const char*>(&asset.header), sizeof(MaterialAssetHeader));

	/* Check if file is good */
	if (!file.good()) return false;

	/* Write material linked assets */
	std::array<const AssetHandle, 4> handles = {
		asset.albedoHandle,
		asset.ormHandle,
		asset.emissiveHandle,
		asset.normalHandle
	};

	static_assert(std::is_trivially_copyable_v<AssetHandle>);

	file.write(reinterpret_cast<const char*>(handles.data()), 4 * sizeof(AssetHandle));

	/* Write uniforms */
	std::array<const Vector4, 2> vectorValues = {
		asset.albedo,
		asset.emissiveColor
	};

	std::array<const float, 3> floatValues = {
		asset.ao,
		asset.roughness,
		asset.metallic
	};

	static_assert(std::is_trivially_copyable_v<Vector4>);

	file.write(reinterpret_cast<const char*>(vectorValues.data()), 2 * sizeof(Vector4));
	file.write(reinterpret_cast<const char*>(floatValues.data()), 3 * sizeof(float));;

	if (!file) return false;

	file.close();

	return true;
}

/**
* Writes a texture asset into a .aeth file
*
* @param filename File name
* @param asset Texture asset data
*
* @returns True if successfully written
*/
bool
AssetManager::SaveTexture(const String& filename, const TextureAsset& asset) {
	/*
		Get the executable path
		TODO: Use project directory
	*/
	String exePath = GetExecutableDir();

	std::filesystem::path filePath = std::filesystem::path(exePath) / filename;

	/* Open file */
	std::ofstream file(filePath, std::ios::binary);

	if (!file.is_open()) {
		Logger::Error("AssetManager::SaveTexture: Failed opening file: {}", filename);
		return false;
	}

	EAssetType type = EAssetType::TEXTURE;

	/* Write TextureAssetHeader */
	uint64_t nRawVersion = TEXTURE_VERSION.Serialize();

	file.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(MAGIC_NUMBER));
	file.write(reinterpret_cast<const char*>(&nRawVersion), sizeof(uint64_t));
	file.write(reinterpret_cast<const char*>(&type), sizeof(EAssetType));

	static_assert(std::is_trivially_copyable_v<TextureAssetHeader>);
	file.write(reinterpret_cast<const char*>(&asset.header), sizeof(TextureAssetHeader));

	/* Check if file is good */
	if (!file.good()) return false;

	file.write(reinterpret_cast<const char*>(asset.buffer.data()), asset.header.nTotalByteSize);

	if (!file) return false;

	file.close();

	return true;
}

/**
* Reads an asset
* 
* @tparam TAsset Asset type
* @tparam THeader Asset header type
* 
* @param filename Asset file name
* @param expectedType Expected asset type
* 
* @returns A handle to the asset
*/
template<typename TAsset, typename THeader>
AssetHandle 
AssetManager::ReadAsset(const String& filename, EAssetType expectedType) {
	AssetHandle handle = { };

	std::ifstream file(filename, std::ios::binary);

	if (!file.is_open()) {
		Logger::Error("AssetManager::ReadAsset: Couldn't open asset {}", filename);
		return handle;
	}

	/* Read global .aeth header */
	uint32_t nMagic = 0;
	uint64_t nRawVersion = 0;
	uint32_t nRawType = 0;

	file.read(reinterpret_cast<char*>(&nMagic), sizeof(nMagic));
	file.read(reinterpret_cast<char*>(&nRawVersion), sizeof(nRawVersion));
	file.read(reinterpret_cast<char*>(&nRawType), sizeof(nRawType));

	/* Check magic number */
	if (nMagic != MAGIC_NUMBER) {
		Logger::Error("AssetManager::ReadAsset: Invalid magic number {}", filename);
		return handle;
	}

	/* Check asset type */
	EAssetType type = static_cast<EAssetType>(nRawType);

	if (type != expectedType) {
		Logger::Error("AssetManager::ReadAsset: Unexpected asset type {}", filename);
		return handle;
	}


	/* Check asset version */
	AssetVersion version = AssetVersion::Deserialize(nRawVersion);

	if (s_assetVersions.contains(expectedType)) {
		AssetVersion expectedVersion = s_assetVersions.at(expectedType);

		if (version != expectedVersion) {
			if (version < expectedVersion) {
				/* TODO: Handle asset version upgrade */
			}
			else {
				Logger::Error("AssetManager::ReadAsset: Asset version is newer than this engine build's last version");
			}
		}
	}

	/* Read asset header */
	THeader header = { };
	file.read((char*)&header, sizeof(THeader));

	return this->ReadAssetData<TAsset, THeader>(filename, file, header);
}

template<>
AssetHandle
AssetManager::ReadAssetData<MeshAsset, MeshAssetHeader>(
	const String& filename,
	std::ifstream& file,
	const MeshAssetHeader& header
) {
	static AssetHandle emptyAsset = AssetHandle{};
	AssetHandle handle = { };

	/* Check mesh total byte size */
	if (header.nSubMeshCount <= 0) {
		Logger::Error(
			"AssetManager::ReadAssetData[MeshAsset]: SubMesh count <= 0. {}",
			static_cast<const char*>(header.displayName)
		);

		file.close();
		return emptyAsset;
	}

	/* Read SubMeshes */
	Vector<SubMeshAsset> subMeshes(header.nSubMeshCount);
	for (uint32_t i = 0; i < header.nSubMeshCount; i++) {
		SubMeshAsset subMesh = { };
		file.read(reinterpret_cast<char*>(&subMesh.header), sizeof(SubMeshAssetHeader));

		subMesh.buffer.resize(subMesh.header.nTotalByteSize);

		std::streamsize beforeBuffer = file.tellg();
		file.read(reinterpret_cast<char*>(subMesh.buffer.data()), subMesh.header.nTotalByteSize);
		std::streamsize afterBuffer = file.tellg();

		uint32_t nReadSize = static_cast<uint32_t>(afterBuffer - beforeBuffer);

		if (nReadSize != subMesh.header.nTotalByteSize) {
			Logger::Error("AssetManager::ReadAssetData[MeshAsset]: Asset file mismatch. Expected {} got {}",
				subMesh.header.nTotalByteSize, nReadSize
			);
		}

		if (!file.good() || !file) {
			Logger::Error("AssetManager::ReadAssetData[MeshAsset]: File error!");
			return emptyAsset;
		}

		subMeshes[i] = std::move(subMesh);
	}

	if (!file) {
		Logger::Error(
			"AssetManager::ReadAssetData[MeshAsset]: Failed reading SubMeshes (expected {})",
			header.nSubMeshCount
		);

		subMeshes.clear();

		return emptyAsset;
	}

	file.close();

	/* Create mesh asset */
	MeshAsset asset = { };
	asset.header = std::move(header);
	asset.subMeshes = std::move(subMeshes);

	this->m_meshCache[filename] = asset;

	handle = AssetHandle::FromPath(filename, EAssetType::MESH);

	this->m_assetCache[handle] = static_cast<AssetVariant>(asset);

	file.close();

	return handle;
}

template<>
AssetHandle
AssetManager::ReadAssetData<SceneAsset, SceneAssetHeader>(
	const String& filename, 
	std::ifstream& file, 
	const SceneAssetHeader& header
) {
	AssetHandle handle = { };
	Vector<GameObjectAsset> objects;

	if (header.nObjectCount > 0) {
		objects.resize(header.nObjectCount);
		file.read(reinterpret_cast<char*>(objects.data()), header.nObjectCount * sizeof(GameObjectAsset));
	}
	
	SceneAsset asset = { };
	asset.header = std::move(header);
	asset.objects = std::move(objects);

	this->m_sceneCache[filename] = asset;

	handle = AssetHandle::FromPath(filename, EAssetType::SCENE);

	this->m_assetCache[handle] = static_cast<AssetVariant>(asset);

	file.close();

	return handle;
}

template<>
AssetHandle
AssetManager::ReadAssetData<TextureAsset, TextureAssetHeader>(
	const String& filename,
	std::ifstream& file,
	const TextureAssetHeader& header
) {
	AssetHandle handle = { };
	Vector<Byte> buffer;

	if (header.nTotalByteSize > 0) {
		buffer.resize(header.nTotalByteSize);
		file.read(reinterpret_cast<char*>(buffer.data()), header.nTotalByteSize);
	}

	TextureAsset asset = { };
	asset.header = std::move(header);
	asset.buffer = std::move(buffer);

	this->m_textureCache[filename] = asset;

	handle = AssetHandle::FromPath(filename, EAssetType::TEXTURE);

	this->m_assetCache[handle] = static_cast<AssetVariant>(asset);

	file.close();

	return handle;
}


template<>
AssetHandle
AssetManager::ReadAssetData<MaterialAsset, MaterialAssetHeader>(
	const String& filename,
	std::ifstream& file,
	const MaterialAssetHeader& header
) {
	AssetHandle handle = { };

	/* Read asset handles */
	std::array<AssetHandle, 4> handles = { };

	file.read(reinterpret_cast<char*>(handles.data()), 4 * sizeof(AssetHandle));

	/* Read vectors values */
	std::array<Vector4, 2> vectorValues = { };
	file.read(reinterpret_cast<char*>(vectorValues.data()), 2 * sizeof(Vector4));

	/* Read float values */
	std::array<float, 3> floatValues = { };
	file.read(reinterpret_cast<char*>(floatValues.data()), 3 * sizeof(float));

	/* Create material asset */
	MaterialAsset asset = { };
	asset.header = std::move(header);
	asset.albedoHandle = handles[0];
	asset.ormHandle = handles[1];
	asset.emissiveHandle = handles[2];
	asset.normalHandle = handles[3];
	asset.albedo = vectorValues[0];
	asset.emissiveColor = vectorValues[1];
	asset.ao = floatValues[0];
	asset.roughness = floatValues[1];
	asset.metallic = floatValues[2];

	this->m_materialCache[filename] = asset;

	handle = AssetHandle::FromPath(filename, EAssetType::MATERIAL);

	this->m_assetCache[handle] = static_cast<AssetVariant>(asset);

	file.close();

	return handle;
}

/**
* Register asset without loading it
* 
* @param path Asset path
* @param type Asset type
*/
AssetHandle 
AssetManager::RegisterAsset(const String& path, EAssetType type) {
	AssetHandle handle = AssetHandle::FromPath(path, type);
	this->m_handleToPath[handle] = path;

	return handle;
}

/**
* Get an asset by its handle
* 
* @param handle Asset handle
*/
const AssetVariant& 
AssetManager::GetAsset(const AssetHandle& handle) {
	/* Check asset cache */
	auto it = this->m_assetCache.find(handle);
	if (it != this->m_assetCache.end()) {
		return it->second;
	}

	/* Try to load if we know the path */
	auto pathIt = this->m_handleToPath.find(handle);
	if (pathIt != this->m_handleToPath.end()) {
		const String& path = pathIt->second;

		switch (handle.type) {
			case EAssetType::MESH:
				this->ReadAsset<MeshAsset, MeshAssetHeader>(path, handle.type);
				break;
			case EAssetType::TEXTURE:
				this->ReadAsset<TextureAsset, TextureAssetHeader>(path, handle.type);
				break;
			case EAssetType::MATERIAL:
				this->ReadAsset<MaterialAsset, MaterialAssetHeader>(path, handle.type);
				break;
			default:
				Logger::Error("AssetManager::GetAsset: Unsupported type");
				break;
		}

		/* Re-check cache after load */
		it = this->m_assetCache.find(handle);
		if (it != this->m_assetCache.end()) {
			return it->second;
		}
	}

	static AssetVariant emptyAsset = { };
	return emptyAsset;
}

/**
* Imports an external asset
* and translates it to 
* Aetherion's custom file
* format (.aeth)
* 
* @param path Asset path
* 
* @returns True if asset sucessfully imported
*/
bool 
AssetManager::ImportAsset(const String& path, const String& projectAssets) {
	fs::path assetPath = path;

	if (!fs::exists(assetPath) || !fs::is_regular_file(assetPath)) {
		Logger::Error("AssetManager::ImportAsset: Can't import folders {}", path);
		return false;
	}

	/* Check if extension is supported */
	String extension = assetPath.extension().string();
	Logger::Debug("AssetManager::ImportAsset: Importing asset with extension {}", extension);

	if (!s_extensionTypes.contains(extension)) {
		Logger::Error("AssetManager::ImportAsset: Unsupported extension type {}", extension);
		return false;
	}

	EImportedAssetType assetType = s_extensionTypes.at(extension);
	String filename = assetPath.filename().stem().string();

	switch (assetType) {
		case EImportedAssetType::MESH:
		{
			/* Import Mesh with Assimp */
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(path.c_str(),
				aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices |
				aiProcess_GenNormals |
				aiProcess_FlipUVs
			);

			if (scene == nullptr) {
				Logger::Error("AssetManager::ImportAsset: Couldn't read mesh file");
				return false;
			}

			uint32_t nNumMeshes = scene->mNumMeshes;

			uint32_t nVertexStride = sizeof(Vertex);

			/* Limit filename to 48 characters */
			if (filename.length() >= 48) {
				filename = filename.substr(0, 48);
			}
			
			MeshAsset meshAsset = { };
			meshAsset.header.displayName = filename;
			meshAsset.header.nSubMeshCount = nNumMeshes;
			meshAsset.subMeshes.resize(nNumMeshes);

			/* Get each mesh from our scene */
			for (uint32_t i = 0; i < nNumMeshes; i++) {
				const aiMesh* pcMesh = scene->mMeshes[i];

				/* Vertices */
				uint32_t nNumVertices = pcMesh->mNumVertices;
				Vector<Vertex> vertices(nNumVertices);
				for (uint32_t v = 0; v < nNumVertices; v++) {
					aiVector3D pos = pcMesh->mVertices[v];
					aiVector3D uv = pcMesh->HasTextureCoords(0)
						? pcMesh->mTextureCoords[0][v]
						: aiVector3D(0.f, 0.f, 0.f);
					aiVector3D normals = pcMesh->HasNormals()
						? pcMesh->mNormals[v]
						: aiVector3D(0.f, 0.f, 0.f);

					vertices[v] = {
						{ pos.x, pos.y, pos.z },
						{ normals.x, normals.y, normals.z },
						{ uv.x, uv.y }
					};
				}

				/* Indices */
				uint32_t nNumIndices = 0;
				for (uint32_t f = 0; f < pcMesh->mNumFaces; f++) {
					nNumIndices += pcMesh->mFaces[f].mNumIndices;
				}

				Vector<uint32_t> indices(nNumIndices);
				uint32_t idx = 0;
				for (uint32_t f = 0; f < pcMesh->mNumFaces; f++) {
					aiFace& face = pcMesh->mFaces[f];
					for (uint32_t j = 0; j < face.mNumIndices; j++) {
						indices[idx++] = face.mIndices[j];
					}
				}

				/* Combine buffers */
				static_assert(std::is_trivially_copyable_v<Vertex>);
				static_assert(std::is_trivially_copyable_v<uint32_t>);

				size_t vertexSize = vertices.size() * sizeof(Vertex);
				size_t indexSize = indices.size() * sizeof(uint32_t);

				size_t totalByteSize = vertexSize + indexSize;
				std::vector<Byte> combined;
				combined.resize(totalByteSize);

				memcpy(combined.data(), vertices.data(), vertexSize);
				memcpy(combined.data() + vertexSize, indices.data(), indexSize);

				/* Check buffer size */
				if (combined.size() != totalByteSize) {
					Logger::Error("AssetManager::ImportAsset: Combined buffer size mismatch. Expected {} got {}",
						totalByteSize,
						combined.size()
					);

					combined.clear();
					vertices.clear();
					return false;
				}

				/* Embedded textures and materials */
				const aiMaterial* pMat = scene->mMaterials[pcMesh->mMaterialIndex];

				std::function<bool(aiTextureType, TextureAsset&)> loadEmbedded =
					[&](aiTextureType type, TextureAsset& out) -> bool {
						aiString texPath;
						if (pMat->GetTextureCount(type) > 0 && pMat->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
							const aiTexture* pTex = scene->GetEmbeddedTexture(texPath.C_Str());

							if (pTex != nullptr) {
								String sanitizedName = texPath.C_Str();
								sanitizedName.erase(
									std::remove(sanitizedName.begin(), sanitizedName.end(), '*'),
									sanitizedName.end()
								);

								out.header.nWidth = pTex->mWidth;
								out.header.nHeight = pTex->mHeight;
								out.header.format = GPUFormat::RGBA8_UNORM;
								out.header.bCompressed = pTex->mHeight == 0;
								out.header.displayName = filename + "_" + sanitizedName;
								out.header.nTotalByteSize = out.header.bCompressed
									? pTex->mWidth
									: pTex->mWidth * pTex->mHeight;

								out.buffer.resize(out.header.nTotalByteSize);

								memcpy(out.buffer.data(), pTex->pcData, out.header.nTotalByteSize);
								return true;
							}
						}
						return false;
					};

				TextureAsset albedoAsset = { };
				TextureAsset ormAsset = { };
				TextureAsset emissiveAsset = { };
				TextureAsset normalAsset = { };

				std::function<AssetHandle(const TextureAsset&, bool)> saveTextureIf =
					[&, this](const TextureAsset& asset, bool bCondition) -> AssetHandle {
						if (bCondition) {
							fs::path texPath = projectAssets;
							texPath /= String(asset.header.displayName) + ".aeth";

							this->SaveTexture(texPath.string(), asset);

							AssetHandle handle = this->RegisterAsset(texPath.string(), EAssetType::TEXTURE);
							return handle;
						}
						return AssetHandle{};
					};

				/* Load textures */
				bool bHasAlbedo = loadEmbedded(aiTextureType_DIFFUSE, albedoAsset);
				bool bHasORM = loadEmbedded(aiTextureType_METALNESS, ormAsset);
				bool bHasEmissive = loadEmbedded(aiTextureType_EMISSIVE, emissiveAsset);
				bool bHasNormal = loadEmbedded(aiTextureType_NORMALS, normalAsset);

				/* Save textures */
				AssetHandle albedoHandle = saveTextureIf(albedoAsset, bHasAlbedo);
				AssetHandle ormHandle = saveTextureIf(ormAsset, bHasORM);
				AssetHandle emissiveHandle = saveTextureIf(emissiveAsset, bHasEmissive);
				AssetHandle normalHandle = saveTextureIf(normalAsset, bHasNormal);

				std::function<EMaterialFlags(bool, EMaterialFlags)> setFlagIf =
					[](bool bCondition, EMaterialFlags flag) -> EMaterialFlags {
						return bCondition ? flag : EMaterialFlags::NONE;
					};

				EMaterialFlags materialFlags = EMaterialFlags::NONE;
				materialFlags = setFlagIf(bHasAlbedo, EMaterialFlags::HAS_ALBEDO_TEXTURE)
						| setFlagIf(bHasORM, EMaterialFlags::HAS_ORM_TEXTURE)
						| setFlagIf(bHasEmissive, EMaterialFlags::HAS_EMISSIVE_TEXTURE)
						| setFlagIf(bHasNormal, EMaterialFlags::HAS_NORMAL_MAP);

				Name subMeshName = filename + "_" + std::to_string(i);

				/* Create material asset */
				MaterialAsset materialAsset = { };
				materialAsset.header.flags = materialFlags;
				materialAsset.header.displayName = String(subMeshName) + "_Mat";
				materialAsset.albedoHandle = albedoHandle;
				materialAsset.ormHandle = ormHandle;
				materialAsset.emissiveHandle = emissiveHandle;
				materialAsset.normalHandle = normalHandle;

				/* Save material asset */
				fs::path materialPath = projectAssets;
				materialPath /= String(materialAsset.header.displayName) + ".aeth";

				this->SaveMaterial(materialPath.string(), materialAsset);
				AssetHandle materialHandle = this->RegisterAsset(materialPath.string(), EAssetType::MATERIAL);

				/* Create sub mesh */
				SubMeshAsset subMesh = { };
				subMesh.header.nVertexCount = nNumVertices;
				subMesh.header.nVertexOffset = 0;
				subMesh.header.nVertexStride = nVertexStride;
				subMesh.header.nIndexCount = nNumIndices;
				subMesh.header.nIndexOffset = 0;
				subMesh.header.nIndexStride = sizeof(uint32_t);
				subMesh.header.materialHandle = materialHandle;
				subMesh.header.nTotalByteSize = combined.size() * sizeof(Byte);
				subMesh.header.displayName = subMeshName;
				subMesh.buffer = std::move(combined);

				meshAsset.subMeshes[i] = subMesh;
			}

			fs::path meshPath = projectAssets;
			meshPath /= filename + ".aeth";

			this->SaveMesh(meshPath.string(), meshAsset);
			AssetHandle meshHandle = this->RegisterAsset(meshPath.string(), EAssetType::MESH);

			break;
		}
		case EImportedAssetType::TEXTURE:
			break;
	}

	return true;
}

/**
* Get asset path from handle
* 
* @param handle Asset handle
* 
* @returns Asset path if registered
*/
String 
AssetManager::GetAssetPath(const AssetHandle& handle) {
	if (!handle.IsValid()) {
		Logger::Error("AssetManager::GetAssetPath: Invalid asset handle");
		return "";
	}

	if (!this->m_handleToPath.contains(handle)) {
		Logger::Warn("AssetManager::GetAssetPath: Asset {} not registered", handle.uuid);
		return "";
	}

	return this->m_handleToPath.at(handle);
}

AssetManager*
AssetManager::GetInstance() {
	if (AssetManager::m_instance == nullptr) {
		AssetManager::m_instance = new AssetManager();
	}

	return AssetManager::m_instance;
}

template AssetHandle AssetManager::ReadAsset<MeshAsset, MeshAssetHeader>(const String&, EAssetType);
template AssetHandle AssetManager::ReadAsset<SceneAsset, SceneAssetHeader>(const String&, EAssetType);
template AssetHandle AssetManager::ReadAsset<TextureAsset, TextureAssetHeader>(const String&, EAssetType);
template AssetHandle AssetManager::ReadAsset<MaterialAsset, MaterialAssetHeader>(const String&, EAssetType);
