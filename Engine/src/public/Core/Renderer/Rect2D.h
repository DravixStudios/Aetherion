#pragma once
#include <cstdint>

struct Offset2D {
	uint32_t x;
	uint32_t y;
};

struct Extent2D {
	uint32_t width;
	uint32_t height;
};

struct Rect2D {
	Offset2D offset;
	Extent2D extent;
};