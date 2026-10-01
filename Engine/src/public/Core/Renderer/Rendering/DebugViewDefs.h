#pragma once
#include <cstdint>

using DebugView = uint16_t;
static constexpr DebugView DEFAULT_VIEW = 0xFFFF;

enum class EDebugViewType {
    DEBUG_GBUFFER = 1,
    DEBUG_LIGHTING = 1 << 1,
    DEFAULT = DEFAULT_VIEW
};

enum class EGBufferDebugView : DebugView {
    ALBEDO = 1,
    NORMAL = 1 << 1,
    ORM = 1 << 2,
    EMISSIVE = 1 << 3,
    BENT_NORMAL = 1 << 4,
    BENT_NORMAL_AO = 1 << 5,
    DEPTH = 1 << 6,
    DEFAULT = DEFAULT_VIEW
};

enum class ELightingDebugView : DebugView {
    DIRECT_SPECULAR = 1,
    INDIRECT_SPECULAR = 1 << 1,
    DIRECT_DIFFUSE = 1 << 2,
    INDIRECT_DIFFUSE = 1 << 3,
    DEFAULT = DEFAULT_VIEW
};