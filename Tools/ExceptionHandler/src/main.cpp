#include <iostream>
#include <chrono>
#include <GLFW/glfw3.h>
#include <thread>
#include <Shared.Common.h>
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
#endif
#if CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <execinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#if CURRENT_PLATFORM(PLATFORM_APPLE)
#include "macOSUtil.h"
#endif

#include <Shared.Common.h>

#include <Heartbeat/HeartbeatMessages.h>

#define WIDTH 800
#define HEIGHT 600

/* Type aliases */
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
using SockAddrIn = SOCKADDR_IN;
using SignedSize = SSIZE_T;
using Socket = SOCKET;
#elif CURRENT_PLATFORM(PLATFORM_LINUX) || CURRENT_PLATFORM(PLATFORM_LINUX) 
using SockAddrIn = struct sockaddr_in;
using SignedSize = ssize_t;
using Socket = int;
#endif

/* Macros */
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
#define SOCKET_VALID(socket) ((socket) != INVALID_SOCKET)
#define CLOSE_SOCKET(socket) (closesocket((socket)))
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
#define SOCKET_VALID(socket) ((socket) >= 0)
#define CLOSE_SOCKET(socket) (close((socket)))
#else
#define SOCKET_VALID(socket) (false)
#define CLOSE_SOCKET(socket) ((void)0)
#endif


#define AETH_SOCK_CHECK(nResult) \
if ((nResult) < 0) { \
    CLOSE_SOCKET(g_sockHandler); \
    return 1; \
}


// TODO: Promote this to a config file
static constexpr uint16_t HANDLER_PORT = 25785;
static constexpr size_t BUFFER_MAX_SIZE = 32; // Size in bytes
static constexpr uint8_t MAX_CONNECTIONS = 1;
static constexpr uint16_t HANG_TIMEOUT_SECONDS = 5;

static constexpr uint16_t TPS_THRESHOLD = 50;

static bool g_bWindowShouldBeVisible = false;

Socket g_sockHandler = -1;

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
/// NO RECOVER ///
    HEARTBEAT = 2,
    HANG = 3,
/// NO RECOVER ///
    CRASH = 4,
    EXIT = 5,
};

struct ClientSocket {
    int nPID = -1;
    int handle = -1;

    uint32_t nCurrentTick = 0;

    uint8_t nTPS = 0;
    EClientState state = EClientState::HELLO; // First message is a HELLO
    bool bShouldClose = false;
};

static void ProcessPacket(ClientSocket& client, void* pPacket);

int main() {
#if CURRENT_PLATFORM(PLATFORM_WINDOWS) // Initialize WinSock2
    WSADATA wsa = { };
    WSAStartup(MAKEWORD(1, 1), &wsa);
#endif

    /* Setup GLFW Window */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* pWindow = glfwCreateWindow(
        WIDTH, HEIGHT,
        "Aetherion Exception Handler",
        nullptr, nullptr);

#if CURRENT_PLATFORM(PLATFORM_APPLE)
    SetBackgroundMode();
#endif

    // TODO: Debug purposes only, hide it and only show it when exception
    glfwHideWindow(pWindow);

    /* Setup socket */
    g_sockHandler = socket(PF_INET, SOCK_STREAM, 0);
    if (!SOCKET_VALID(g_sockHandler)) {
        spdlog::error("Failed initializing Exception handler socket");
        return 1;
    }

    /* Bind and listen */
    SockAddrIn addr = { };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HANDLER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // WinSock2 SOCKADDR_IN doesn't have sin_len attribute
#if CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
    addr.sin_len = sizeof(addr);
#endif

    AETH_SOCK_CHECK(bind(g_sockHandler, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    AETH_SOCK_CHECK(listen(g_sockHandler, MAX_CONNECTIONS));

    /* Accept new client */
    struct sockaddr_in clientAddr = { };
    socklen_t clientSize = sizeof(clientAddr);

    ClientSocket client = { };
    client.handle = accept(g_sockHandler, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
    AETH_SOCK_CHECK(client.handle);

#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
    DWORD timeout = HANG_TIMEOUT_SECONDS * 1000;
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
    struct timeval timeout = {};
    timeout.tv_sec = HANG_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
#endif

    if (setsockopt(
        client.handle,
        SOL_SOCKET,
        SO_RCVTIMEO,
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
        reinterpret_cast<const char*>(&timeout),
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
        &timeout,
#endif
        sizeof(timeout)) < 0
    ) {
        std::cerr << "Failed to set socket timeout" << std::endl;
        return 1;
    }

    uint16_t nMsPerTick = 0;

    while (!client.bShouldClose) {
        std::vector<char> buffer(BUFFER_MAX_SIZE);
        const SignedSize bufferSize = recv(client.handle, buffer.data(), BUFFER_MAX_SIZE, 0);

        // Buffer size: (0, BUFFER_MAX_SIZE]
        ASSERT_DESC(bufferSize != 0, "Buffer size was zero");
        if (bufferSize == 0 || bufferSize > BUFFER_MAX_SIZE) {
            client.bShouldClose = true;
            break;
        }

        if (bufferSize < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                client.state = EClientState::HANG;
                continue;
            }

            client.bShouldClose = true;
            client.state = EClientState::EXIT;
            break;
        }

        void* pPacket = malloc(bufferSize);

        if (pPacket == nullptr) {
            client.bShouldClose = true;
            break;
        }

        memcpy(pPacket, buffer.data(), bufferSize);
        buffer.clear();

        /*
         * Treat the first packet as a HELLO
         */
        if (client.state == EClientState::HELLO) {
            const HelloPacket hello = *(static_cast<HelloPacket*>(pPacket));

            ASSERT_DESC(hello.nPID > 0, "PID is less or equal to 0");
            ASSERT_DESC(hello.nTPS > 0, "TPS is less or equal to 0");

            client.nPID = hello.nPID;
            client.nTPS = hello.nTPS;

            nMsPerTick = 1000 / client.nTPS;

            /* Forward to a heartbeat state */
            client.state = EClientState::HEARTBEAT;

            free(pPacket);
            continue;
        }

        ProcessPacket(client, pPacket);

        free(pPacket);

        std::chrono::nanoseconds tickInterval =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(nMsPerTick - TPS_THRESHOLD));
        std::this_thread::sleep_for(tickInterval);
    }

    // Cleanup socket
    CLOSE_SOCKET(client.handle);

#if CURRENT_PLATFORM(PLATFORM_WINDOWS) // WinSock2 specific cleanup
    WSACleanup();
#endif

#if CURRENT_PLATFORM(PLATFORM_APPLE)
    SetForegroundMode();
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    while (g_bWindowShouldBeVisible) {

    }

    return 0;
}

void
ProcessPacket(ClientSocket& client, void* pPacket) {
    switch (client.state) {
        case EClientState::HEARTBEAT:
            const HeartbeatPacket heartbeat = *(static_cast<HeartbeatPacket*>(pPacket));
            const uint32_t nClientTick = client.nCurrentTick;

            /* TODO: Handle hangs */
            if (heartbeat.nTick == (nClientTick + 1)) {
                client.nCurrentTick++;
            } else {
                exit(1);
            }
            break;
    }
}