#pragma once
#include <string>
#include <filesystem>

#include "Core/Logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

#include "Core/Containers.h" 

#define VK_CHECK(res, msg) \
    do {\
        impl::vk_check_impl(res, CLASS_NAME, __func__, msg); \
    } while(0)

/* 
    Implementations we don't want to be visible from anywhere. 

    Note: If for some reason we want to use these functions, we'll
    need to access them like impl::foo_impl(x);
*/
namespace impl {
    inline void 
    vk_check_impl(
        VkResult res,
        const char* className,
        const char* func,
        const char* msg
    ) {
        if (res != VK_SUCCESS) {
            Logger::Error("{}::{}: {}", className, func, msg);
            throw std::runtime_error(msg);
        }
    }
}

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct ScreenQuadVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

struct WVP {
	glm::mat4 World;
	glm::mat4 View;
	glm::mat4 Projection;
};

struct ObjectInstanceData {
    uint32_t wvpOffset;
    uint32_t textureIndex;
    uint32_t ormTextureIndex;
    uint32_t emissiveTextureIndex;
};

struct DrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int vertexOffset;
    uint32_t firstInstance;
};

struct DrawBatch {
    uint32_t indexCount;
    uint32_t firstIndex;
    int vertexOffset;
    uint32_t instanceDataIndex;
};

struct FrameIndirectData {
    uint32_t instanceDataOffset;
    uint32_t batchDataOffset;
    uint32_t indirectDrawOffset;
    uint32_t wvpOffset;
    uint32_t objectCount;
};

struct CollectedDrawData {
    Vector<ObjectInstanceData> instances;
    Vector<DrawBatch> batches;
    Vector<WVP> wvps;
    uint32_t nTotalBatches = 0;

    glm::mat4 viewProj = glm::mat4(1.f);
    glm::mat4 view = glm::mat4(1.f);
    glm::mat4 proj = glm::mat4(1.f);
    glm::vec3 cameraPosition = glm::vec3(0.f);
};

inline String GetExecutableDir() {
    char buffer[4096];

#if defined(_WIN32)
    GetModuleFileNameA(NULL, buffer, sizeof(buffer));
    std::filesystem::path exePath(buffer);
    return exePath.parent_path().string();

#elif defined(__APPLE__)
    uint32_t size = sizeof(buffer);
    _NSGetExecutablePath(buffer, &size);
    std::filesystem::path exePath = std::filesystem::canonical(buffer);
    return exePath.parent_path().string();

#elif defined(__linux__)
    ssize_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (count != -1) {
        buffer[count] = '\0';
        std::filesystem::path exePath(buffer);
        return exePath.parent_path().string();
    }
	Logger::Error("GetExecutableDir: Couldn't determine executable path on Linux");
    throw std::runtime_error("GetExecutableDir: Couldn't determine executable path on Linux");

#else
	Logger::Error("GetExecutableDir: Unsupported platform");
    throw std::runtime_error("GetExecutableDir: Unsupported platform");
#endif
}