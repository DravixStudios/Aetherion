#include "Core/Resources/AssetManager.h"
#include <fstream>

AssetManager* AssetManager::m_instance;

AssetManager::AssetManager() {}

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

	file.write(reinterpret_cast<const char*>(&asset.header), sizeof(MeshAssetHeader));

	/* Check if file is good */
	if (!file.good()) return false;

	/* Copy MeshAsset data to file */
	if (asset.buffer.size() != asset.header.nTotalByteSize) {
		Logger::Error("AssetManager::WriteMesh: Size mismatch");
		return false;
	}

	file.write(reinterpret_cast<const char*>(asset.buffer.data()), asset.header.nTotalByteSize);

	if (!file) return false;

	file.close();

	return true;
}

/**
* Read a Mesh asset from a
* .aeth asset file
*/
AssetHandle 
AssetManager::ReadMesh(const String& filename) {
	AssetHandle handle = { };

	/* Read file */
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		Logger::Error("AssetManager::ReadMesh: Couldn't open {} file", filename);
		return handle;
	}

	/* Read file header */
	uint32_t nMagic = 0;
	uint64_t nVersion = 0;
	uint32_t nRawType = 0;

	file.read(reinterpret_cast<char*>(&nMagic), sizeof(nMagic));
	file.read(reinterpret_cast<char*>(&nVersion), sizeof(nVersion));
	file.read(reinterpret_cast<char*>(&nRawType), sizeof(nRawType));

	/* Check magic number */
	if (nMagic != MAGIC_NUMBER) {
		Logger::Error("AssetManager::ReadMesh: Mesh asset file {} has no valid magic number", filename);
		return handle;
	}

	/* Check if is a mesh */
	EAssetType type = static_cast<EAssetType>(nRawType);

	if (type != EAssetType::MESH) {
		Logger::Error("AssetManager::ReadMesh: Not a mesh file. {}", filename);
		file.close();
		return handle;
	}

	/* Check asset version */
	AssetVersion version = AssetVersion::Deserialize(nVersion);
	if (version != MESH_VERSION) {
		/* TODO: Implement a asset version update system */
	}

	/* Read asset header */
	MeshAssetHeader header = { };
	file.read(reinterpret_cast<char*>(&header), sizeof(MeshAssetHeader));

	if (header.nTotalByteSize <= 0) {
		Logger::Error("AssetManager::ReadMesh: Mesh asset size is 0. {}", static_cast<const char*>(header.displayName));
		file.close();
		return handle;
	}

	/* Read asset data */
	Vector<Byte> buffer(header.nTotalByteSize);
	file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

	if (!file) {
		Logger::Error(
			"AssetManager::ReadMesh: Failed reading mesh data (expected {} bytes, got {})",
			buffer.size(),
			file.gcount()
		);

		buffer.clear();

		return handle;
	}

	file.close();

	/* Create mesh asset */
	MeshAsset asset = { };
	asset.header = std::move(header);
	asset.buffer = std::move(buffer);

	this->m_meshCache[filename] = asset;

	handle = AssetHandle::FromPath(filename, EAssetType::MESH);

	this->m_assetCache[handle] = static_cast<AssetVariant>(asset);

	return handle;
}

/**
* Register asset without loading it
* 
* @param path Asset path
* @param type Asset type
*/
void 
AssetManager::RegisterAsset(const String& path, EAssetType type) {
	AssetHandle handle = AssetHandle::FromPath(path, type);
	this->m_handleToPath[handle] = path;
}

/**
* Get asset path from handle
* and load asset
* 
* @param path Asset path
* 
* @returns Requested mesh asset
*/
MeshAsset&
AssetManager::GetMesh(const String& path) {
	/* Check if mesh stored on the cache */
	if (!this->m_meshCache.contains(path)) {
		/* Create a handle from path */
		AssetHandle handle = AssetHandle::FromPath(path, EAssetType::MESH);

		/* If handle found, load the mesh */
		if (this->m_handleToPath.contains(handle)) {
			this->ReadMesh(path);
			return this->m_meshCache[path];
		}

		static MeshAsset emptyMesh = { };
		return emptyMesh;
	}

	return this->m_meshCache[path];
}

/**
* Get an asset by it's handle
* 
* @param handle Asset handle
*/
const AssetVariant& 
AssetManager::GetAsset(const AssetHandle& handle) {
	if (this->m_assetCache.contains(handle)) return this->m_assetCache[handle];

	if (this->m_handleToPath.contains(handle)) {
		const String& path = this->m_handleToPath[handle];

		if (handle.type == EAssetType::MESH) {
			this->ReadMesh(path);

			return this->m_assetCache[handle];
		}
	}

	static AssetVariant emptyAsset = { };
	return emptyAsset;
}

AssetManager*
AssetManager::GetInstance() {
	if (AssetManager::m_instance == nullptr) {
		AssetManager::m_instance = new AssetManager();
	}

	return AssetManager::m_instance;
}