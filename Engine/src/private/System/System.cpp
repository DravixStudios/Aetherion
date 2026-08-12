#include "System/System.h"
#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#endif

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
    write(STDERR_FILENO, "Crashed\nCallstack:\n", 11);

    void* stack[128];
    int nFrames = backtrace(stack, 64);

    backtrace_symbols_fd(
        stack,
        nFrames,
        STDERR_FILENO
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