#include "System/System.h"
#if defined(__APPLE__) || defined(__linux__)
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
