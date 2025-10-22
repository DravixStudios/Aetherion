#pragma once
#include <iostream>

/* Forward declarations */
class Core;
class Renderer;

class GPURingBuffer {
public:
    GPURingBuffer();
	~GPURingBuffer() = default;

    virtual void Init(uint32_t nBufferSize, uint32_t nAlignment, uint32_t nFramesInFlight);
    virtual void* Allocate(uint32_t nDataSize, uint64_t& outOffset);
    virtual uint32_t Align(uint32_t nValue, uint32_t nAlignment);
    virtual void Reset(uint32_t nImageIndex);

    virtual uint64_t GetSize();
protected:
    Core* m_core;
    Renderer* m_renderer;
    uint32_t m_nBufferSize;
    uint32_t m_nAlignment;
    uint32_t m_nFramesInFlight;
};