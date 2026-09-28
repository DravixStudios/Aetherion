#pragma once
#include <iostream>
#include "Utils.h"

#include <Shared.Common.h>

#if CURRENT_PLATFORM(PLATFORM_WINDOWS)
using Socket = uintptr_t;
#elif CURRENT_PLATFORM(PLATFORM_APPLE) || CURRENT_PLATFORM(PLATFORM_LINUX)
using Socket = int;
#endif

namespace System {
    int GetSelfPID();
    void InstallExceptionHandler();

    void InitializeHeartbeatSocket();
    int SpawnProcess(const String& executable);

    static Socket heartbeatSocket = 0;
}