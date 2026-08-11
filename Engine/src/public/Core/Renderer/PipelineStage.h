#pragma once
#include <cstdint>

enum class EPipelineStage : uint32_t {
	TOP_OF_PIPE = 1,
	DRAW_INDIRECT = 1 << 1,
	VERTEX_INPUT = 1 << 2,
	VERTEX_SHADER = 1 << 3,
	TESSELLATION_CONTROL_SHADER = 1 << 4,
	TESSELLATION_EVALUATION_SHADER = 1 << 5,
	GEOMETRY_SHADER = 1 << 6,
	FRAGMENT_SHADER = 1 << 7,
	FRAGMENT = FRAGMENT_SHADER,
	EARLY_FRAGMENT_TESTS = 1 << 8,
	LATE_FRAGMENT_TESTS = 1 << 9,
	COLOR_ATTACHMENT_OUTPUT = 1 << 10,
	COMPUTE_SHADER = 1 << 11,
	TRANSFER = 1 << 12,
	BOTTOM_OF_PIPE = 1 << 13,
	HOST = 1 << 14,
	ALL_GRAPHICS = 1 << 15,
	ALL_COMMANDS = 1 << 16,

	/* Aliases for backward compatibility with Device.h names */
	TESSELLATION_CONTROL = TESSELLATION_CONTROL_SHADER,
	TESSELLATION_EVAL = TESSELLATION_EVALUATION_SHADER,
	GEOMETRY = GEOMETRY_SHADER
};

inline EPipelineStage operator|(EPipelineStage a, EPipelineStage b) {
	return static_cast<EPipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EPipelineStage operator&(EPipelineStage a, EPipelineStage b) {
	return static_cast<EPipelineStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
