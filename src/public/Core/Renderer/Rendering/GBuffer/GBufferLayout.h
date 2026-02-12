#pragma once
#include <cstdint>
#include "Core/Renderer/GPUFormat.h"

namespace GBufferLayout {
	constexpr GPUFormat ALBEDO = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat NORMAL = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat ORM = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat EMISSIVE = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat POSITION = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat BENT_NORMAL = GPUFormat::RGBA16_FLOAT;
	constexpr GPUFormat DEPTH = GPUFormat::D32_FLOAT;
	constexpr uint32_t COLOR_COUNT = 6;
}