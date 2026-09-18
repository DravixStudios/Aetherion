#pragma once
#include <cstdint>
// TODO: Contemplate no-spdlog use case
#include <spdlog/spdlog.h>

#if defined(_MSC_VER)
#define PLATFORM_WINDOWS 1
#else
#define PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
#define PLATFORM_APPLE 1
#else
#define PLATFORM_APPLE 0
#endif

#if defined(__linux__)
#define PLAFTORM_LINUX 1
#else
#define PLATFORM_LINUX 0
#endif

#define CURRENT_PLATFORM(platform) ((platform) != 0)

#define MAYBE_UNUSED [[maybe_unused]]

#if defined(USING_CMAKE)
#if !defined(NDEBUG)
#define IS_DEBUG_BUILD 1
#endif
#elif defined(_MSC_VER)
#if defined(_DEBUG)
#define IS_DEBUG_BUILD 1
#endif
#else
#define IS_DEBUG_BUILD 0
#endif

#if IS_DEBUG_BUILD
    #if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
    #elif defined(__APPLE__)
    #define DEBUG_BREAK() __builtin_debugtrap();
    #elif defined(__linux__)
    #include <signal.g>
    #define DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace DebugAssert {

    inline void Assert(
        const char* condition,
        const char* file,
        const uint32_t nLine,
        const bool bShouldBreak = false,
        const char* msg = nullptr
    ) {
        spdlog::error(
            "Assert failed: Condition: {}. File: {}:{}. Desc: {}",
            condition,
            file,
            nLine,
            msg ? msg : ""
        );

        if (bShouldBreak)
            DEBUG_BREAK();
    }

}

#define ASSERT(cond) \
        do { \
            if (!(cond)) { \
                DebugAssert::Assert(#cond, __FILE__, __LINE__, true); \
            } \
        } while (false)

#define ASSERT_DESC(cond, msg) \
        do { \
            if (!(cond)) { \
                DebugAssert::Assert(#cond, __FILE__, __LINE__, true, msg); \
            } \
        } while (false)
#else
#define ASSERT(cond) ((void)0)
#define ASSERT_DESC(cond, msg) ((void)0)
#endif
