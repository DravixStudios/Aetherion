#pragma once
#include "Core/Containers.h"
#include "Core/Resources/AssetHandle.h"

struct MeshAssetHeader {
	uint32_t nVertexCount;
	uint32_t nVertexOffset;
	uint32_t nVertexStride;
	uint32_t nIndexCount;
	uint32_t nIndexOffset;
	uint32_t nIndexStride;
	uint32_t nTotalByteSize;
	AssetHandle materialHandle;
	Name displayName;
};

struct MeshAsset {
	MeshAssetHeader header;
	Vector<Byte> buffer;
};