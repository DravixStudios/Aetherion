#pragma once
#include "Utils.h"
#include "Core/Renderer/Pipeline.h"

enum class ESamplerFlags {
	SUBSAMPLED = 1,
	SUBSAMPLED_COARSE_RECONSTRUCTION = 1 << 1,
	DESCRIPTOR_BUFFER_CAPTURE_REPLAY = 1 << 2,
	NON_SEAMLESS_CUBE_MAP = 1 << 3,
	IMAGE_PROCESSING_QCOM = 1 << 4
};

inline ESamplerFlags
operator|(ESamplerFlags a, ESamplerFlags b) {
	return static_cast<ESamplerFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ESamplerFlags
operator&(ESamplerFlags a, ESamplerFlags b) {
	return static_cast<ESamplerFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class EFilter {
	NEAREST,
	LINEAR,
	CUBIC,
	CUBIC_IMG
};

enum class EMipmapMode {
	MIPMAP_MODE_NEAREST,
	MIPMAP_MODE_LINEAR
};

enum class EAddressMode {
	REPEAT,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MIRROR_CLAMP_TO_EDGE,
};

enum class EBorderColor {
	FLOAT_TRANSPARENT_BLACK,
	INT_TRANSPARENT_BLACK,
	FLOAT_OPAQUE_BLACK,
	INT_OPAQUE_BLACK,
	FLOAT_OPAQUE_WHITE,
	INT_OPAQUE_WHITE,
	FLOAT_CUSTTOM,
	INT_CUSTOM
};

struct SamplerCreateInfo {
	ESamplerFlags flags;
	EFilter magFilter;
	EFilter minFilter;
	EMipmapMode mipmapMode;
	
	EAddressMode addressModeU = EAddressMode::REPEAT;
	EAddressMode addressModeV = EAddressMode::REPEAT;
	EAddressMode addressModeW = EAddressMode::REPEAT;

	float mipLodBias = 0.f;
	bool bAnisotropyEnable = true;
	float maxAnisotropy = 16.f;

	bool bCompareEnable = false;
	ECompareOp compareOp = ECompareOp::NEVER;

	float minLod = 0.f;
	float maxLod = 0.f;

	EBorderColor borderColor = EBorderColor::INT_OPAQUE_BLACK;
	bool bUnnormalizedCoordinates = false;
};

class Sampler {
public:
	using Ptr = Ref<Sampler>;

	virtual ~Sampler() = default;

	virtual void Create(SamplerCreateInfo createInfo) = 0;
};