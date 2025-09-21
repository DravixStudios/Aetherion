# CMakeList.txt: proyecto de CMake para Aetherion, incluya el origen y defina
# la lógica específica del proyecto aquí.
#

# Agregue un origen al ejecutable de este proyecto.
add_executable ("Aetherion" "main.cpp" "public/Core/Core.h" "private/Core/Core.cpp"
        public/Core/Renderer/Renderer.h
        private/Core/Renderer/Renderer.cpp
        public/Core/Renderer/VulkanRenderer.h "private/Core/Renderer/VulkanRenderer.cpp" "public/Utils.h" "public/Core/Renderer/GPUBuffer.h" "public/Core/Renderer/VulkanBuffer.h" "private/Core/Renderer/VulkanBuffer.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
if(WIN32)
    find_package(unofficial-shaderc CONFIG REQUIRED)
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog unofficial::shaderc::shaderc glm::glm)
endif()
if(NOT WIN32)
    set(SHADERC_INCLUDE_DIR /usr/local/include)
    set(SHADERC_LIBRARIES /usr/local/lib/libshaderc_combined.a)

    include_directories(${SHADERC_INCLUDE_DIR})
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog ${SHADERC_LIBRARIES} glm::glm)

endif()

set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")

target_include_directories(Aetherion PRIVATE "${PRIVATE_INCLUDES}")
include_directories("$ENV{VULKAN_SDK}/Include")