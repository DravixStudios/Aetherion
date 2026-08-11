#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GPUFormat.h"

struct TextureAssetHeader {
	uint32_t nWidth;
	uint32_t nHeight;
	uint32_t nTotalByteSize;
	bool bCompressed = false;
	GPUFormat format;
	Name displayName;
};

struct TextureAsset {
	TextureAssetHeader header;
	Vector<Byte> buffer;
};