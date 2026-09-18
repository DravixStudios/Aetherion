#include <iostream>
#include <chrono>
#include <GLFW/glfw3.h>
#include <thread>
#include <Shared.Common.h>
#if CURRENT_PLATFORM(PLATFORM_APPLE) || defined(__linux__)
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

#define AETH_SOCK_CHECK(nResult) \
if ((nResult) < 0) { \
    close(g_sockHandler); \
    return 1; \
}

#define AETH_CLIENT_ASSERT(condition, msg) \
do { \
    if (!(condition)) { \
        std::cerr << "ASSERT FAILED: " << msg << '\n'; \
    } \
} while (false)

// TODO: Promote this to a config file
static constexpr uint16_t HANDLER_PORT = 25785;
static constexpr size_t BUFFER_MAX_SIZE = 32; // Size in bytes
static constexpr uint8_t MAX_CONNECTIONS = 1;
static constexpr uint16_t HANG_TIMEOUT_SECONDS = 5;

static constexpr uint16_t TPS_THRESHOLD = 50;

int g_sockHandler = -1;

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
    /* Setup GLFW Window */
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* pWindow = glfwCreateWindow(
        WIDTH, HEIGHT,
        "Aetherion Exception Handler",
        nullptr, nullptr);

    SetBackgroundMode();

    // TODO: Debug purposes only, hide it and only show it when exception
    glfwHideWindow(pWindow);

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

    ClientSocket client = { };
    client.handle = accept(g_sockHandler, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
    AETH_SOCK_CHECK(client.handle);

    struct timeval timeout = {};
    timeout.tv_sec = HANG_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    if (setsockopt(
        client.handle,
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)) < 0
    ) {
        std::cerr << "Failed to set socket timeout" << std::endl;
        return 1;
    }

    uint16_t nMsPerTick = 0;

    while (!client.bShouldClose) {
        std::vector<char> buffer(BUFFER_MAX_SIZE);
        const size_t bufferSize = recv(client.handle, buffer.data(), BUFFER_MAX_SIZE, 0);

        // Buffer size: (0, BUFFER_MAX_SIZE]
        ASSERT_DESC(bufferSize <= 0, "Buffer size has an invalid size");
        if (bufferSize <= 0 || bufferSize > BUFFER_MAX_SIZE) {
            client.bShouldClose = true;
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

            AETH_CLIENT_ASSERT(hello.nPID <= 0, "PID is less or equal to 0");
            AETH_CLIENT_ASSERT(hello.nTPS <= 0, "TPS is less or equal to 0");

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

    // Cleanup
    close(client.handle);

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
            return;
    }
}