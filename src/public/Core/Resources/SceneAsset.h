#pragma once
#include "Core/Containers.h"
#include "Core/Resources/GameObjectAsset.h"

struct SceneAssetHeader {
	uint32_t nObjectCount = 0;
	Name displayName;
};

struct SceneAsset {
	SceneAssetHeader header;
	Vector<GameObjectAsset> objects;
};