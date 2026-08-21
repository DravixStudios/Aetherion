#include <iostream>
#include <GLFW/glfw3.h>
#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <csignal>
#include <execinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include <Heartbeat/HeartbeatMessages.h>

#define WIDTH 800
#define HEIGHT 600

#define AETH_SOCK_CHECK(nResult) \
if ((nResult) < 0) { \
    close(g_sockHandler); \
    return 1; \
}

// TODO: Promote this to a config file
static constexpr uint16_t HANDLER_PORT = 25785;
static constexpr uint8_t MAX_CONNECTIONS = 1;

int g_sockHandler = -1;

bool g_bQuit = false;

// TODO: Client state machine
/*
 * EClientState describes the state of the client.
 *
 * By logic, the HANG state is the only one that can recover
 * to a previous state. After a crash, the state can only increment
 * by index (e.g: CRASH->EXIT).
 *
 * NOTE: The HELLO State should be only used for the first packet
 */
enum class EClientState : uint8_t {
    HELLO = 1,
    HEARTBEAT = 2,
    HANG = 3,
    CRASH = 4,
    EXIT = 5,
    STATE_COUNT
};

struct ClientSocket {
    int nPID = -1;
    int handle = -1;

    uint32_t nCurrentTick = 0;

    uint8_t nTPS = 0;
    EClientState state = EClientState::HELLO; // First message is a HELLO
};

int main() {
    /* Setup GLFW Window */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* pWindow = glfwCreateWindow(
        WIDTH, HEIGHT,
        "Aetherion Exception Handler",
        nullptr, nullptr);

    // TODO: Debug purposes only, hide it and only show it when exception
    glfwShowWindow(pWindow);

    /* Setup socket */
    AETH_SOCK_CHECK(g_sockHandler = socket(PF_INET, SOCK_STREAM, 0));

    if (g_sockHandler < 0) {
        std::cerr << "Exception handler failed to initialize socket" << std::endl;
        return 1;
    }

    /* Bind and listen */
    struct sockaddr_in addr = { };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HANDLER_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    AETH_SOCK_CHECK(bind(g_sockHandler, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    AETH_SOCK_CHECK(listen(g_sockHandler, MAX_CONNECTIONS));

    /* Accept new client */
    struct sockaddr_in clientAddr = { };
    socklen_t clientSize = sizeof(clientAddr);

    const int clientSock = accept(g_sockHandler, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
    AETH_SOCK_CHECK(clientSock);

    while (!g_bQuit) {

    }

    return 0;
}