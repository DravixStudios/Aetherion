#pragma once
#include <iostream>
#include "Utils.h"

namespace System {
    int GetSelfPID();
    void InstallExceptionHandler();

    void InitializeHeartbeatSocket();

    static int heartbeatSocket = 0;
}