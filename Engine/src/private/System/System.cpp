#include "System/System.h"
#include <thread>
#include <chrono>
#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <csignal>
#include <execinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

static constexpr uint16_t HANDLER_PORT = 25785;
static constexpr uint8_t TICKS_PER_SECOND = 1;

bool g_bQuitThread = false;
uint32_t g_nCurrentTick = 0;

std::thread heartbeatThread{};

struct HelloPacket {
    int nPID = -1;
    uint8_t nTPS = -1;
};

struct HeartbeatPacket {
    uint32_t nTick = -1;
};

void HeartbeatThread(int socket);

int
System::GetSelfPID() {
    // TODO: Windows use-case
    int nPID = -1;
#if defined(__APPLE__) || defined(__linux__)
    const pid_t pid = getpid();
    nPID = static_cast<int>(pid);
#endif
    return nPID;
}

// TODO: Windows use-case
#if defined(__APPLE__) || defined(__linux__)
static void
ExceptionHandler(int nSignal, siginfo_t* pInfo, void* pvContext) {
    // TODO: Develop this
    void* stack[128];
    int nFrames = backtrace(stack, 64);

    char** ppBacktrace = backtrace_symbols(
        stack,
        nFrames
    );



    _exit(128 + nSignal);
}
#endif

void
System::InstallExceptionHandler() {
    // TODO: Windows use-case
#if defined(__APPLE__) || defined(__linux__)
    struct sigaction action = { .sa_sigaction = ExceptionHandler };
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &action, nullptr);
    sigaction(SIGABRT, &action, nullptr);
    sigaction(SIGFPE, &action, nullptr);
    sigaction(SIGILL, &action, nullptr);
    sigaction(SIGBUS, &action, nullptr);
#endif
}

void
System::InitializeHeartbeatSocket() {
    // TODO: Windows use-case
    System::heartbeatSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (System::heartbeatSocket < 0) {
        Logger::Error("System::InitializeHeartbeatSocket: Failed initializing heartbeat socket");
        return;
    }

    struct sockaddr_in addr = { };
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(HANDLER_PORT);
    addr.sin_family = AF_INET;

    int nRes = connect(System::heartbeatSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    if (nRes != 0) {
        Logger::Error("System::InitializeHeartbeatSocket: Failed connecting to the Exception handler process");
        return;
    }

    const HelloPacket helloPacket = { .nPID = GetSelfPID(), .nTPS = TICKS_PER_SECOND };

    send(System::heartbeatSocket, &helloPacket, sizeof(HelloPacket), 0);

    heartbeatThread = std::thread(HeartbeatThread, System::heartbeatSocket);
    heartbeatThread.detach();
}

void
HeartbeatThread(int socket) {
    const uint16_t msPerTick = 1000 / TICKS_PER_SECOND;
    while (!g_bQuitThread) {
        std::chrono::nanoseconds tickInterval =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(msPerTick));
        std::this_thread::sleep_for(tickInterval);

        const HeartbeatPacket heartbeat = { .nTick = g_nCurrentTick++ };
        send(socket, &heartbeat, sizeof(HeartbeatPacket), 0);
    }
}