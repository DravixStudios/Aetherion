#pragma once
#include <iostream>
#include "Core/Containers.h"

#include "Core/Renderer/GPUBuffer.h"

struct RingBufferCreateInfo {
    uint32_t nBufferSize;
    uint32_t nAlignment;
    uint32_t nFramesInFlight;
    EBufferUsage usage = EBufferUsage::UNIFORM_BUFFER;
};

class GPURingBuffer {
public:
    using Ptr = Ref<GPURingBuffer>;

    static constexpr const char* CLASS_NAME = "GPURingBuffer";

	virtual ~GPURingBuffer() = default;

    virtual void Create(const RingBufferCreateInfo& createInfo) = 0;
    virtual void* Allocate(uint32_t nDataSize, uint32_t& outOffset) = 0;
    virtual uint32_t Align(uint32_t nValue, uint32_t nAlignment) = 0;
    virtual void Reset(uint32_t nImageIndex) = 0;

    virtual uint64_t GetSize() const = 0;
    virtual uint32_t GetAlignment() const { return this->m_nAlignment; }

    Ref<GPUBuffer> GetBuffer() const { return this->m_buffer; }

    uint32_t GetPerFrameSize() const { return this->m_nPerFrameSize; }

protected:
    Ref<GPUBuffer> m_buffer;
    uint32_t m_nPerFrameSize = 0;

    uint32_t m_nBufferSize;
    uint32_t m_nAlignment;
    uint32_t m_nFramesInFlight;
    EBufferUsage m_usage;
};