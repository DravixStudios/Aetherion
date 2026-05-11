#pragma once
#include "Core/Containers.h"

struct MeshAssetHeader {
	uint32_t nVertexCount;
	uint32_t nVertexOffset;
	uint32_t nVertexStride;
	uint32_t nIndexCount;
	uint32_t nIndexOffset;
	uint32_t nIndexStride;
	uint32_t nTotalByteSize;
	uint32_t nNameSize;
	Name displayName;
};

struct MeshAsset {
	MeshAssetHeader header;
	Vector<Byte> buffer;
};