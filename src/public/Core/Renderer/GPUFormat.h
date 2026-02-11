#pragma once

enum class GPUFormat {
	RGBA8_UNORM = 1 << 1,
	RGBA8_SRGB = 1 << 2,
	RGBA16_FLOAT = 1 << 3,
	RGBA32_FLOAT = 1 << 4,
	D24_UNORM_S8_UINT =  1 << 5,
	D32_FLOAT = 1 << 6,
	D32_FLOAT_S8_UINT = 1 << 7,
	R8_UNORM = 1 << 8,
	R16_FLOAT = 1 << 9,
	UNKNOWN_FORMAT = 0xFF
};