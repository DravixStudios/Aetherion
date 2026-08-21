#pragma once

struct HelloPacket {
    int nPID = -1;
    uint8_t nTPS = -1;
};

struct HeartbeatPacket {
    uint32_t nTick = 0;
};