#pragma once

enum class ExceptionType {
    SEG_FAULT,
    ABORTED,
    FLOATING_POINT_ERROR,
    ILLEGAL_INSTRUCTION,
    BUS_ERROR
};

struct HelloPacket {
    int nPID = -1;
    uint8_t nTPS = -1;
};

struct HeartbeatPacket {
    uint32_t nTick = 0;
};