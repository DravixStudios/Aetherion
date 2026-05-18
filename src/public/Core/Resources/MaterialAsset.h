#pragma once
#include "Core/Containers.h"
#include "Core/Resources/AssetHandle.h"
#include "Math/Vector4.h"

enum class EMaterialFlags : uint32_t {
    HAS_ALBEDO_TEXTURE = 1,
    HAS_ORM_TEXTURE = 1 << 1,
    HAS_EMISSIVE_TEXTURE = 1 << 2,
    HAS_NORMAL_MAP = 1 << 3
};

inline EMaterialFlags
operator|(EMaterialFlags a, EMaterialFlags b) {
    return static_cast<EMaterialFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EMaterialFlags
operator&(EMaterialFlags a, EMaterialFlags b) {
    return static_cast<EMaterialFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

struct MaterialAssetHeader {
    EMaterialFlags flags;
    Name displayName;
};

struct MaterialAsset {
    MaterialAssetHeader header;

    AssetHandle albedoHandle;
    AssetHandle ormHandle;
    AssetHandle emissiveHandle;
    AssetHandle normalHandle;

    Vector4 albedo;
    float ao = 0.f;
    float roughness = .5f;
    float metallic = .5f;
    Vector4 emissiveColor;
};
