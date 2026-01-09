file(GLOB_RECURSE SOURCES "private/**/*.cpp" "main.cpp" "private/stb_image_impl.cpp")
file(GLOB_RECURSE HEADERS "public/**/*.h")
add_executable("Aetherion" ${SOURCES} ${HEADERS})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Aetherion PROPERTY CXX_STANDARD 20)
endif()

target_compile_definitions(Aetherion PRIVATE $<$<BOOL:${LOGGING_USE_SPDLOG}>:LOGGING_USE_SPDLOG>)

set("PRIVATE_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/public")
set("PUBLIC_INCLUDES" "${CMAKE_CURRENT_SOURCE_DIR}/../include" )

target_sources(Aetherion PRIVATE "${PUBLIC_INCLUDES}/miniz/miniz.c")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(spirv_cross_c_shared REQUIRED)

if(WIN32)
    find_package(unofficial-shaderc CONFIG REQUIRED)
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog unofficial::shaderc::shaderc glm::glm assimp::assimp spirv-cross-c-shared)
    target_compile_definitions(Aetherion PRIVATE NOMINMAX)
endif()
if(NOT WIN32)
    set(SHADERC_INCLUDE_DIR /usr/local/include)
    set(SHADERC_LIBRARIES /usr/local/lib/libshaderc_combined.a)

    include_directories(${SHADERC_INCLUDE_DIR})
    target_link_libraries(Aetherion PRIVATE Vulkan::Vulkan glfw spdlog::spdlog ${SHADERC_LIBRARIES} glm::glm assimp::assimp spirv-cross-c-shared)

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