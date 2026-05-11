#pragma once
#include "Core/Containers.h"
#include "Math/Transform.h"
#include "Core/Resources/AssetHandle.h"
 
/* Asset component structure */
enum class EAssetComponent : uint32_t{
	MESH = 1,
	
	/* TODO */
	ANIMATOR = 1 << 1,
	BOX_COLLIDER = 1 << 2,
	CAPSULE_COLLIDER = 1 << 3,
	SPHERE_COLLIDER = 1 << 4
};

inline EAssetComponent
operator|(EAssetComponent a, EAssetComponent b) {
	return static_cast<EAssetComponent>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EAssetComponent
operator&(EAssetComponent a, EAssetComponent b) {
	return static_cast<EAssetComponent>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/* GameObject asset structure */
struct GameObjectAssetHeader {
	Name displayName;
};

struct GameObjectAsset {
	GameObjectAssetHeader header;
	Transform transform;

	EAssetComponent components;

	/* 
		GameObject components.

		If the object does not have
		any of the components enabled
		at 'componentes' member, that
		component will be skipped
	*/
	AssetHandle meshHandle;
	AssetHandle animatorHandle;
	AssetHandle boxColliderHandle;
	AssetHandle capsuleColliderHandle;
	AssetHandle sphereColliderHandle;
};