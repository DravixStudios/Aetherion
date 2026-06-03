#pragma once
#include <string>
#include <filesystem>

#include "Core/Logger.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <nanosvg/nanosvg.h>
#include <nanosvg/nanosvgrast.h>

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

#define VK_SET_NAME(device, objectType, objectHandle, name) \
    do {\
        impl::vk_set_obj_name(device, objectType, objectHandle, name.c_str()); \
    } while(0)

/* 
    Implementations we don't want to be visible from anywhere. 

    Note: If for some reason we want to use these functions, we'll
    need to access them like impl::foo_impl(x);
*/
namespace impl {
#ifndef NDEBUG
    inline PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectName = nullptr;

    /**
    * Initialize debug utils
    * 
    * REMINDER: This function will only be compiled in
    * debug mode, and it must be called only by 
    * VulkanRenderer. 
    * 
    * @param instance Vulkan instance
    */
    inline void init_debug_utils(VkInstance instance) {
        if(pfnSetDebugUtilsObjectName == nullptr) {
            pfnSetDebugUtilsObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT")
            );
        }
    }
#endif // NDEBUG

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

    inline void
    vk_set_obj_name(
        VkDevice device,
        VkObjectType type, 
        uint64_t handle,
        const char* name
    ) {
#ifndef NDEBUG
        if (!pfnSetDebugUtilsObjectName) return;

        VkDebugUtilsObjectNameInfoEXT nameInfo = { };
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.objectHandle = handle;
        nameInfo.pObjectName = name;
        nameInfo.pNext = nullptr;

        pfnSetDebugUtilsObjectName(device, &nameInfo);
#endif // NDEBUG
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
    uint32_t materialOffset;
};

struct MaterialInstanceData {
    uint32_t albedoIndex;
    uint32_t ormIndex;
    uint32_t emissiveIndex;
    uint32_t normalIndex;

    glm::vec4 albedoColor;
    glm::vec4 emissiveColor;
    float ao;
    float roughness;
    float metallic;

    uint32_t materialFlags;
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
    uint32_t nBlockIdx;
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
    Vector<MaterialInstanceData> materials;
    Vector<DrawBatch> batches;
    Vector<WVP> wvps;
    uint32_t nTotalBatches = 0;

    glm::mat4 viewProj = glm::mat4(1.f);
    glm::mat4 view = glm::mat4(1.f);
    glm::mat4 proj = glm::mat4(1.f);
    glm::vec3 cameraPosition = glm::vec3(1.f);
};

struct FrustumData {
    glm::mat4 viewProj = glm::mat4(1.f);
    glm::vec4 frustumPlanes[6];
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

inline uint32_t 
NextPowerOf2(uint32_t x) {
    if (x == 0) return 1;

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
}

inline Vector<unsigned char> 
LoadSVG(const char* pcSVG, float dpi, uint32_t& nOutWidth, uint32_t& nOutHeight) {
    Vector<char> svgBuff(pcSVG, pcSVG + strlen(pcSVG) + 1);

    NSVGimage* pImage = nsvgParse(svgBuff.data(), "px", dpi);
    if (!pImage) {
        Logger::Error("LoadSVG: Failed to parse SVG");
        nOutWidth = 0;
        nOutHeight = 0;
        return Vector<unsigned char>();
    }

    int nWidth = static_cast<uint32_t>(pImage->width);
    int nHeight = static_cast<uint32_t>(pImage->height);

    Vector<unsigned char> bitmap(nWidth * nHeight * 4);

    NSVGrasterizer* pRaster = nsvgCreateRasterizer();
    nsvgRasterize(pRaster, pImage, 0, 0, 1.f, bitmap.data(), nWidth, nHeight, nWidth * 4);
    nsvgDeleteRasterizer(pRaster);
    nsvgDelete(pImage);

    nOutWidth = nWidth;
    nOutHeight = nHeight;

    return bitmap;
}