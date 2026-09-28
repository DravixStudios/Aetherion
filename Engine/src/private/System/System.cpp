#include <Shared.Common.h>

#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dbghelp.lib")
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
#include <unistd.h>
#include <csignal>
#include <execinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <spawn.h>
#include <sys/wait.h>

extern char** environ;
#endif

#include <thread>
#include <chrono>

#include "System/System.h"
#include <Heartbeat/HeartbeatMessages.h>

// TODO: Promote this to a config file
static constexpr uint16_t HANDLER_PORT = 25785;
static constexpr uint8_t TICKS_PER_SECOND = 8;
static constexpr uint16_t GRACE_PERIOD_SECONDS = 2;

static bool g_bQuitThread = false;
static uint32_t g_nCurrentTick = 0;

static std::thread heartbeatThread{};

static void HeartbeatThread(Socket socket);

int
System::GetSelfPID() {
    // TODO: Windows use-case
    int nPID = -1;
#if CURRENT_PLATFORM(PLATFORM_APPLE) || defined(__linux__)
    const pid_t pid = getpid();
    nPID = static_cast<int>(pid);
#endif
    return nPID;
}

#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
static LONG WINAPI
ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    if (exceptionInfo == nullptr || exceptionInfo->ExceptionRecord == nullptr)
        return EXCEPTION_EXECUTE_HANDLER;

    const DWORD dwCode = exceptionInfo->ExceptionRecord->ExceptionCode;
    const void* pvExceptionAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

    ExceptionType type = ExceptionType::ABORTED;

    // Translate exception type
    switch (dwCode) {
        case EXCEPTION_ACCESS_VIOLATION:
            type = ExceptionType::SEG_FAULT;
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            type = ExceptionType::ILLEGAL_INSTRUCTION;
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            type = ExceptionType::FLOATING_POINT_ERROR;
            break;
    }

    const HANDLE hProcess = GetCurrentProcess();
    const HANDLE hThread = GetCurrentThread();

    CONTEXT* pContext = exceptionInfo->ContextRecord;

    SymInitialize(hProcess, nullptr, TRUE);

    // Get Stack frame depending on the arch
    STACKFRAME64 frame = { };
#if defined(_M_X64)
    frame.AddrPC.Offset = pContext->Rip;
    frame.AddrFrame.Offset = pContext->Rbp;
    frame.AddrStack.Offset = pContext->Rsp;
    
    const DWORD dwMachineType = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
    frame.AddrPC.Offset = pContext->Eip;
    frame.AddrFrame.Offset = pContext->Ebp;
    frame.AddrStack.Offset = pContext->Esp;

    const DWORD dwMachineType = IMAGE_FILE_MACHINE_I386;
#endif

    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;

    uint64_t stack[128];
    uint16_t nFrameCount = 0;

    while (nFrameCount < 128) {
        if (!StackWalk64(
            dwMachineType, 
            hProcess, hThread,
            &frame,
            reinterpret_cast<PVOID>(pContext), 
            nullptr, 
            SymFunctionTableAccess64, 
            SymGetModuleBase64, 
            nullptr
        )) {
            break;
        }

        if (frame.AddrPC.Offset == 0)
            break;

        stack[nFrameCount++] = frame.AddrPC.Offset;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
static void
ExceptionHandler(int nSignal, siginfo_t* pInfo, void* pvContext) {
    // TODO: Develop this
    void* stack[128];
    int nFrames = backtrace(stack, 64);

    MAYBE_UNUSED char** ppBacktrace = backtrace_symbols(
        stack,
        nFrames
    );



    _exit(128 + nSignal);
}
#endif

void
System::InstallExceptionHandler() {
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
    SetUnhandledExceptionFilter(ExceptionHandler);
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
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
#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
    WSADATA wsa = { };
    WSAStartup(MAKEWORD(1, 1), &wsa);
#endif
    System::heartbeatSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (System::heartbeatSocket < 0) {
        Logger::Error("System::InitializeHeartbeatSocket: Failed initializing heartbeat socket");
        return;
    }

    struct sockaddr_in addr = { };
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(HANDLER_PORT);
    addr.sin_family = AF_INET;

    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(GRACE_PERIOD_SECONDS);

    for (auto now = std::chrono::steady_clock::now();
        now < deadline;
        now = std::chrono::steady_clock::now()) {

        const int nRes = connect(System::heartbeatSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

        if (nRes == 0) {
            Logger::Error("System::InitializeHeartbeatSocket: Failed connecting to the Exception handler process");
            return;
        }
    }

    const HelloPacket helloPacket = { .nPID = GetSelfPID(), .nTPS = TICKS_PER_SECOND };

    send(System::heartbeatSocket, reinterpret_cast<const char*>(&helloPacket), sizeof(HelloPacket), 0);

    heartbeatThread = std::thread(HeartbeatThread, System::heartbeatSocket);
    heartbeatThread.detach();
}

void
HeartbeatThread(Socket socket) {
    static constexpr uint16_t msPerTick = 1000 / TICKS_PER_SECOND;
    while (!g_bQuitThread) {
        std::chrono::nanoseconds tickInterval =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(msPerTick));
        std::this_thread::sleep_for(tickInterval);

        const HeartbeatPacket heartbeat = { .nTick = g_nCurrentTick++ };
        send(socket, reinterpret_cast<const char*>(&heartbeat), sizeof(HeartbeatPacket), 0);
    }
}

#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
int
System::SpawnProcess(const String& executable) {
    STARTUPINFO startupInfo = { };
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo = { };
    
    char commandLine[MAX_PATH] = { };
    strncpy_s(commandLine, executable.c_str(), _TRUNCATE);

    const BOOL bResult = CreateProcess(
        nullptr, 
        commandLine,
        nullptr, 
        nullptr,
        FALSE, 
        0,
        nullptr,
        nullptr, 
        &startupInfo, 
        &processInfo
    );

    ASSERT_DESC(bResult, "Failed spawning process");

    /* Cleanup */
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return 0;
}
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || defined(__linux__)
int
System::SpawnProcess(const String& executable) {
    pid_t pid = 0;

    char* const args[] = {
        const_cast<char*>(executable.c_str()),
        nullptr
    };

    int nResult = posix_spawn(
        &pid,
        executable.c_str(),
        nullptr,
        nullptr,
        args,
        environ
    );

    if (nResult != 0) {
        Logger::Error("System::SpawnProcess: Failed spawning posix process");
        return nResult;
    }

    Logger::Debug("System::SpawnProcess: Spawned process with PID: {}", pid);

    return nResult;
}
#endif