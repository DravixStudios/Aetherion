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
MeshAsset 
AssetManager::ReadMesh(const String& filename) {
	MeshAsset asset = { };

	/* Read file */
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		Logger::Error("AssetManager::ReadMesh: Couldn't open {} file", filename);
		return asset;
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
		return asset;
	}

	/* Check if is a mesh */
	EAssetType type = static_cast<EAssetType>(nRawType);

	if (type != EAssetType::MESH) {
		Logger::Error("AssetManager::ReadMesh: Not a mesh file. {}", filename);
		file.close();
		return asset;
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
		return asset;
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

		return asset;
	}

	asset.header = std::move(header);
	asset.buffer = std::move(buffer);

	file.close();

	return asset;
}

/**
* Get mesh from cache
* or load it
* 
* @param path Asset path
* 
* @returns Requested mesh asset
*/
MeshAsset&
AssetManager::GetMesh(const String& path) {
	if(!this->m_meshCache.contains(path)) {
		this->m_meshCache[path] = this->ReadMesh(path);
	}

	return this->m_meshCache[path];
}

AssetManager*
AssetManager::GetInstance() {
	if (AssetManager::m_instance == nullptr) {
		AssetManager::m_instance = new AssetManager();
	}

	return AssetManager::m_instance;
}