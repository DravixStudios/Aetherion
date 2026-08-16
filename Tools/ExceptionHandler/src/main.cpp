#include <iostream>
#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* pWindow = glfwCreateWindow(
        WIDTH, HEIGHT,
        "Aetherion Exception Handler",
        nullptr, nullptr);

    glfwShowWindow(pWindow);

    while (!glfwWindowShouldClose(pWindow)) {
        glfwPollEvents();
    }

    return 0;
}