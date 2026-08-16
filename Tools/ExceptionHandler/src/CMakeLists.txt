add_executable("AethExceptionHandler" "main.cpp")

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries("AethExceptionHandler" PRIVATE glfw)