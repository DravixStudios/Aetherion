file(GLOB_RECURSE SOURCES "private/**/*.cpp" "main.cpp" "private/stb_image_impl.cpp" "private/tinyexr_impl.cpp")
file(GLOB_RECURSE HEADERS "public/**/*.h")
add_executable("Aetherion" ${SOURCES} ${HEADERS})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

target_compile_definitions(Aetherion PRIVATE $<$<BOOL:${LOGGING_USE_SPDLOG}>:LOGGING_USE_SPDLOG>)
target_compile_definitions(Aetherion PRIVATE $<$<BOOL:${RENDERER_USE_VULKAN}>:RENDERER_USE_VULKAN>)


set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")
set("PUBLIC_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/../include" )

target_sources(Aetherion PRIVATE "${PUBLIC_INCLUDES}/miniz/miniz.c")

# Fetch SPIRV-Cross
include(FetchContent)

FetchContent_Declare(
    spirv-cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG vulkan-sdk-1.4.335.0
)

FetchContent_MakeAvailable(spirv-cross)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

# Build imgui
file(GLOB "IMGUI_SRC" 
    "${PUBLIC_INCLUDES}/imgui/*.cpp"
    "${PUBLIC_INCLUDES}/imgui/*.h"
    "${PUBLIC_INCLUDES}/imgui/bckends/*.cpp"
    "${PUBLIC_INCLUDES}/imgui/bckends/*.h"
)

add_library("imgui" STATIC ${IMGUI_SRC})

target_link_libraries(Aetherion PRIVATE spirv-cross-c spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl xxHash::xxhash)
target_link_libraries(Aetherion PRIVATE imgui)

if(WIN32)
    find_package(unofficial-shaderc CONFIG REQUIRED)
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog unofficial::shaderc::shaderc glm::glm assimp::assimp)
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