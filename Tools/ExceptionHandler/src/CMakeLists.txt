add_executable("AethExceptionHandler" "main.cpp")

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries("AethExceptionHandler" PRIVATE glfw)

if(APPLE)
    target_sources(AethExceptionHandler PRIVATE "macOSUtil.h" "macOSUtil.mm")
    target_link_libraries(
            AethExceptionHandler
            PRIVATE
            "-framework Cocoa"
    )
endif()

target_include_directories(AethExceptionHandler PRIVATE "${SHARED_INCLUDES}")