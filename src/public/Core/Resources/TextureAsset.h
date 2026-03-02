#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/GPUFormat.h"

struct TextureAssetHeader {
	uint32_t nWidth;
	uint32_t nHeight;
	uint32_t nTotalByteSize;
	GPUFormat format;
	Name displayName;
};

struct TextureAsset {
	TextureAssetHeader header;
	const void* pcData = nullptr;
};