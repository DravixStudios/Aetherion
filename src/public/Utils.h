#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <string>
#include <filesystem>
#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(__linux__)
    #include <unistd.h>
#endif

#include "Core/Containers.h" 

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
	spdlog::error("GetExecutableDir: Couldn't determine executable path on Linux");
    throw std::runtime_error("GetExecutableDir: Couldn't determine executable path on Linux");

#else
	spdlog::error("GetExecutableDir: Unsupported platform");
    throw std::runtime_error("GetExecutableDir: Unsupported platform");
#endif
}
