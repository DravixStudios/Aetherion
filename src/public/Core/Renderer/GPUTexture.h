#pragma once
#include <iostream>

#include "Core/Containers.h"
#include "Core/Renderer/Extent3D.h"
#include "Core/Renderer/GPUFormat.h"
#include "Core/Renderer/GPUBuffer.h"

enum class ETextureType {
	TEXTURE,
	CUBEMAP,
	UNDEFINED
};

enum class ETextureDimensions {
	TYPE_1D,
	TYPE_2D,
	TYPE_3D
};

enum class ETextureFlags : uint32_t {
	SPARSE_BINDING = 1,
	SPARSE_RESIDENCY = 1 << 1,
	SPARSE_ALIASED = 1 << 2,
	MUTABLE_FORMAT = 1 << 3,
	CUBE_COMPATIBLE = 1 << 4
};

inline ETextureFlags 
operator|(ETextureFlags a, ETextureFlags b) {
	return static_cast<ETextureFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ETextureFlags
operator&(ETextureFlags a, ETextureFlags b) {
	return static_cast<ETextureFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class ESampleCount {
	SAMPLE_1 = 1,
	SAMPLE_2 = 2,
	SAMPLE_4 = 4,
	SAMPLE_8 = 8,
	SAMPLE_16 = 16,
	SAMPLE_32 = 32,
	SAMPLE_64 = 64
};

enum class ETextureTiling {
	OPTIMAL,
	LINEAR
};

enum class ETextureUsage : uint32_t {
	TRANSFER_SRC = 1,
	TRANSFER_DST = 1 << 2,
	SAMPLED = 1 << 3,
	STORAGE = 1 << 4,
	COLOR_ATTACHMENT = 1 << 5,
	DEPTH_STENCIL_ATTACHMENT = 1 << 6,
	TRANSIENT_ATTACHMENT = 1 << 7,
	INPUT_ATTACHMENT = 1 << 8
};

inline ETextureUsage
operator|(ETextureUsage a, ETextureUsage b) {
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ETextureUsage
operator&(ETextureUsage a, ETextureUsage b) {
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class ESharingMode {
	EXCLUSIVE,
	CONCURRENT
};

enum class ETextureLayout {
	UNDEFINED,
	GENERAL,
	COLOR_ATTACHMENT_OPTIMAL,
	DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	DEPTH_STENCIL_READ_ONLY_OPTIMAL,
	SHADER_READ_ONLY_OPTIMAL,
	TRANSFER_SRC_OPTIMAL,
	TRANSFER_DST_OPTIMAL,
	PREINITIALIZED
};

struct TextureCreateInfo {
	Ref<GPUBuffer> buffer;
	ETextureFlags flags;
	ETextureDimensions imageType;
	GPUFormat format;
	Extent3D extent;
	uint32_t nMipLevels = 1;
	uint32_t nArrayLayers = 1;
	ESampleCount samples = ESampleCount::SAMPLE_1;
	ETextureTiling tiling = ETextureTiling::OPTIMAL;
	ETextureUsage usage = ETextureUsage::COLOR_ATTACHMENT;
	ESharingMode sharingMode = ESharingMode::EXCLUSIVE;
	uint32_t nQueueFamilyIndexCount = 0;
	const uint32_t* pQueueFamilyIndices = nullptr;
	ETextureLayout initialLayout = ETextureLayout::UNDEFINED;
};

class GPUTexture {
public:
	static constexpr const char* CLASS_NAME = "GPUTexture";
	
	using Ptr = Ref<GPUTexture>;

	virtual ~GPUTexture() = default;

	/**
	* Creates a GPU texture
	* 
	* @param createInfo Texture create info
	*/
	virtual void Create(const TextureCreateInfo& createInfo) = 0;

	virtual uint32_t GetSize() const = 0;

	virtual void Reset() = 0;

	ETextureType textureType = ETextureType::UNDEFINED;
};