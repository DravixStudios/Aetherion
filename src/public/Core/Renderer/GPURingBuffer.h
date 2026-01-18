#pragma once
#include <iostream>
#include "Core/Containers.h"

#include "Core/Renderer/GPUBuffer.h"

struct RingBufferCreateInfo {
    uint32_t nBufferSize;
    uint32_t nAlignment;
    uint32_t nFramesInFlight;
    EBufferType bufferType = EBufferType::CONSTANT_BUFFER;
};

class GPURingBuffer {
public:
    using Ptr = Ref<GPUBuffer>;

    static constexpr const char* CLASS_NAME = "GPURingBuffer";

	virtual ~GPURingBuffer() = default;

    virtual void Create(const RingBufferCreateInfo& createInfo) = 0;
    virtual void* Allocate(uint32_t nDataSize, uint32_t& outOffset) = 0;
    virtual uint32_t Align(uint32_t nValue, uint32_t nAlignment) = 0;
    virtual void Reset(uint32_t nImageIndex) = 0;

    virtual uint64_t GetSize() const = 0;
    virtual uint32_t GetAlignment() const { return this->m_nAlignment; }
protected:
    uint32_t m_nBufferSize;
    uint32_t m_nAlignment;
    uint32_t m_nFramesInFlight;
    EBufferType m_bufferType;
};