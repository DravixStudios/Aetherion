# CMakeList.txt: proyecto de CMake para Aetherion, incluya el origen y defina
# la lógica específica del proyecto aquí.
#

# Agregue un origen al ejecutable de este proyecto.
add_executable ("Aetherion" "main.cpp" "public/Core/Core.h" "private/Core/Core.cpp"
        public/Core/Renderer/Renderer.h
        private/Core/Renderer/Renderer.cpp
        public/Core/Renderer/VulkanRenderer.h)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")

target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog)
target_include_directories(Aetherion PRIVATE "${PRIVATE_INCLUDES}")
include_directories("$ENV{VULKAN_SDK}/Include")