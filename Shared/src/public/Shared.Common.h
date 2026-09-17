#pragma once
#include <cstdint>
// TODO: Contemplate no-spdlog use case
#include <spdlog/spdlog.h>

#ifndef NDEBUG
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
