#pragma once
#include <iostream>
#include <xxhash.h>

enum class EAssetType : uint32_t {
	MESH = 0x01,
	TEXTURE = 0x02,
	MATERIAL = 0x03,
	GAMEOBJECT = 0x04,
	SCENE = 0x05,
	SHADER = 0x06,
	UNDEFINED = 0xFF
};

struct AssetHandle {
	uint64_t uuid = 0;
	EAssetType type = EAssetType::UNDEFINED;

	bool
	operator>(const AssetHandle& other) const {
		return this->uuid > other.uuid;
	}

	bool
	operator<(const AssetHandle& other) const {
		return this->uuid < other.uuid;
	}

	bool
	operator==(const AssetHandle& other) const {
		return this->uuid == other.uuid;
	}

	static AssetHandle
	FromPath(const String& path, EAssetType type) {
		AssetHandle handle = { };
		handle.uuid = XXH64(path.c_str(), path.size(), 0);
		handle.type = type;
		return handle;
	}

	bool
	IsValid() const {
		return this->uuid != 0 && this->type != EAssetType::UNDEFINED;
	}
};