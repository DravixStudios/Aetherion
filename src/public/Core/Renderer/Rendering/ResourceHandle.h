#pragma once
#include <cstdint>

struct TextureHandle {
	uint32_t nIndex = ~0U;
	uint32_t nVersion = 0;
	bool IsValid() const { return nIndex != ~0U; }
};