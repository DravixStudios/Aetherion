#pragma once
#include <cstdint>

struct TextureHandle {
	uint32_t nIndex = ~0U;
	uint32_t nVersion = 0;
	bool IsValid() const { return nIndex != ~0U; }

	bool operator<(const TextureHandle& other) const {
		if (nIndex != other.nIndex) return nIndex < other.nIndex;
		return nVersion < other.nVersion;
	}

	bool operator==(const TextureHandle& other) const {
		return nIndex == other.nIndex && nVersion == other.nVersion;
	}

	bool operator!=(const TextureHandle& other) const {
		return !(*this == other);
	}
};