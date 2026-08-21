#pragma once
#include <iostream>
#include "Utils.h"

namespace System {
    int GetSelfPID();
    void InstallExceptionHandler();

    void InitializeHeartbeatSocket();
    int SpawnProcess(const String& executable);

    static int heartbeatSocket = 0;
}