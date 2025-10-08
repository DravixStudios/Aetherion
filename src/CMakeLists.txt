add_executable ("Aetherion" "main.cpp" "public/Core/Core.h" "private/Core/Core.cpp"
        "public/Core/Renderer/Renderer.h"
        "private/Core/Renderer/Renderer.cpp"
        "private/Core/Renderer/Vulkan/VulkanRenderer.cpp" "public/Utils.h" "public/Core/Renderer/GPUBuffer.h" "public/Core/Renderer/Vulkan/VulkanBuffer.h" "private/Core/Renderer/Vulkan/VulkanBuffer.cpp" "public/Core/Scene/Scene.h" "private/Core/Scene/Scene.cpp" "public/Core/GameObject/GameObject.h" "private/Core/GameObject/GameObject.cpp" "public/Core/Scene/SceneManager.h" "private/Core/Scene/SceneManager.cpp" "public/Math/Vector3.h" "private/Math/Vector3.cpp" "public/Math/Transform.h" "private/Math/Transform.cpp" "public/Core/GameObject/Components/Component.h" "private/Core/GameObject/Components/Component.cpp" "public/Core/GameObject/Components/Mesh.h" "private/Core/GameObject/Components/Mesh.cpp" "private/stb_image_impl.cpp" "public/Core/Renderer/GPUTexture.h" "public/Core/Renderer/Vulkan/VulkanTexture.h" "private/Core/Renderer/Vulkan/VulkanTexture.cpp" "public/Core/Renderer/GPUFormat.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
if(WIN32)
    find_package(unofficial-shaderc CONFIG REQUIRED)
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog unofficial::shaderc::shaderc glm::glm assimp::assimp )
endif()
if(NOT WIN32)
    set(SHADERC_INCLUDE_DIR /usr/local/include)
    set(SHADERC_LIBRARIES /usr/local/lib/libshaderc_combined.a)

    include_directories(${SHADERC_INCLUDE_DIR})
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog ${SHADERC_LIBRARIES} glm::glm assimp::assimp)

endif()

set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")
set("PUBLIC_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/../include" )

target_include_directories(Aetherion PRIVATE "${PRIVATE_INCLUDES}")
target_include_directories(Aetherion PUBLIC "${PUBLIC_INCLUDES}")
include_directories("$ENV{VULKAN_SDK}/Include")