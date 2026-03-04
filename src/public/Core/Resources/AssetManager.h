#pragma once
#include "Core/Containers.h"
#include "Utils.h"

#include "Core/Resources/MeshAsset.h"
#include "Core/Resources/TextureAsset.h"

/* File magic number and version */
static constexpr uint32_t MAGIC_NUMBER = 0x48544541; // AETH
static constexpr uint32_t VERSION = 1;

enum class AssetType : uint32_t {
	MESH = 0x01,
	TEXTURE = 0x02,
	MATERIAL = 0x03
};

class AssetManager {
public:
	AssetManager();

	~AssetManager() = default;

	bool SaveMesh(const String& filename, const MeshAsset& asset);
	MeshAsset ReadMesh(const String& filename);

	static AssetManager* GetInstance();
private:
	

	static AssetManager* m_instance;
};