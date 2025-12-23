add_executable ("Aetherion" "main.cpp" "public/Core/Core.h" "private/Core/Core.cpp"
        "public/Core/Renderer/Renderer.h"
        "private/Core/Renderer/Renderer.cpp"
        "private/Core/Renderer/Vulkan/VulkanRenderer.cpp" "public/Utils.h" "public/Core/Renderer/GPUBuffer.h" "public/Core/Renderer/Vulkan/VulkanBuffer.h" "private/Core/Renderer/Vulkan/VulkanBuffer.cpp" "public/Core/Scene/Scene.h" "private/Core/Scene/Scene.cpp" "public/Core/GameObject/GameObject.h" "private/Core/GameObject/GameObject.cpp" "public/Core/Scene/SceneManager.h" "private/Core/Scene/SceneManager.cpp" "public/Math/Vector3.h" "private/Math/Vector3.cpp" "public/Math/Transform.h" "private/Math/Transform.cpp" "public/Core/GameObject/Components/Component.h" "private/Core/GameObject/Components/Component.cpp" "public/Core/GameObject/Components/Mesh.h" "private/Core/GameObject/Components/Mesh.cpp" "private/stb_image_impl.cpp" "public/Core/Renderer/GPUTexture.h" "public/Core/Renderer/Vulkan/VulkanTexture.h" "private/Core/Renderer/Vulkan/VulkanTexture.cpp" "public/Core/Renderer/GPUFormat.h" "public/Core/Renderer/ResourceManager.h" "private/Core/Renderer/ResourceManager.cpp" "public/Core/Renderer/GPURingBuffer.h" "public/Core/Renderer/Vulkan/VulkanRingBuffer.h" "private/Core/Renderer/Vulkan/VulkanRingBuffer.cpp" "private/Core/Renderer/GPURingBuffer.cpp" "public/Core/Camera/Camera.h" "private/Core/Camera/Camera.cpp" "public/Core/Input.h" "private/Core/Input.cpp" "public/Core/Camera/EditorCamera.h" "private/Core/Camera/EditorCamera.cpp" "public/Core/Time.h" "private/Core/Time.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")
set("PUBLIC_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/../include" )

target_sources(Aetherion PRIVATE "${PUBLIC_INCLUDES}/miniz/miniz.c")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
if(WIN32)
    find_package(unofficial-shaderc CONFIG REQUIRED)
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog unofficial::shaderc::shaderc glm::glm assimp::assimp )
    target_compile_definitions(Aetherion PRIVATE NOMINMAX)
endif()
if(NOT WIN32)
    set(SHADERC_INCLUDE_DIR /usr/local/include)
    set(SHADERC_LIBRARIES /usr/local/lib/libshaderc_combined.a)

    include_directories(${SHADERC_INCLUDE_DIR})
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog ${SHADERC_LIBRARIES} glm::glm assimp::assimp)

endif()

if(APPLE)
  message(STATUS "Configuring Vulkan/MoltenVK for macOS")
  target_link_libraries(Aetherion PRIVATE "$ENV{VULKAN_SDK}/lib/libMoltenVK.dylib")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(Aetherion PRIVATE "${PRIVATE_INCLUDES}")
target_include_directories(Aetherion PUBLIC "${PUBLIC_INCLUDES}")
include_directories("$ENV{VULKAN_SDK}/Include")