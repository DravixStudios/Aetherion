#pragma once
#include "Core/Containers.h"
#include "Utils.h"

#include "Core/Resources/MeshAsset.h"
#include "Core/Resources/TextureAsset.h"

/* Asset version structure */
struct AssetVersion {
	uint16_t major = 1;
	uint16_t minor = 0;
	uint16_t patch = 0;

	AssetVersion() = default;
	constexpr AssetVersion(uint16_t maj, uint16_t min, uint16_t patch) : major(maj), minor(min), patch(patch) {}

	bool 
	operator==(const AssetVersion& other) const {
		return this->major == other.major && this->minor == other.minor && this->patch == other.patch;
	}

	bool 
	operator!=(const AssetVersion& other) const {
		return !(*this == other);
	}

	bool
	operator<(const AssetVersion& other) const {
		if (this->major != other.major) return this->major < other.major;
		if (this->minor != other.minor) return this->minor < other.minor;
		return this->patch < other.patch;
	}

	bool 
	operator>(const AssetVersion& other) const {
		return other < *this;
	}

	bool 
	operator<=(const AssetVersion& other) const {
		return !(*this > other);
	}

	bool 
	operator>=(const AssetVersion& other) const {
		return !(*this < other);
	}

	uint64_t
	Serialize() const {
		return (static_cast<uint64_t>(this->major) << 32) |
			(static_cast<uint64_t>(this->minor) << 16) |
			static_cast<uint64_t>(this->patch);
	}
	
	static constexpr AssetVersion 
	Deserialize(uint64_t nValue) {
		AssetVersion v = { };

		v.major = static_cast<uint16_t>((nValue >> 32) & 0xFFFF);
		v.minor = static_cast<uint16_t>((nValue >> 16) & 0xFFFF);
		v.patch = static_cast<uint16_t>(nValue & 0xFFFF);

		return v;
	}
};

/* Different asset type versions */
static constexpr AssetVersion MESH_VERSION(1, 0, 0);
static constexpr AssetVersion TEXTURE_VERSION(1, 0, 0);
static constexpr AssetVersion MATERIAL_VERSION(1, 0, 0);
static constexpr AssetVersion GAMEOBJECT_VERSION(1, 0, 0);

/* File magic number */
static constexpr uint32_t MAGIC_NUMBER = 0x48544541; // AETH

enum class EAssetType : uint32_t {
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

	MeshAsset& GetMesh(const String& path);

	static AssetManager* GetInstance();
private:
	static AssetManager* m_instance;

	Map<String, MeshAsset> m_meshCache;
	Map<String, TextureAsset> m_textureCache;
};