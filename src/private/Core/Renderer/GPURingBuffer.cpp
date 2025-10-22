#include "Core/Renderer/GPURingBuffer.h"
#include "Core/Core.h"

GPURingBuffer::GPURingBuffer() {
	this->m_core = Core::GetInstance();
	this->m_renderer = this->m_core->GetRenderer();
	this->m_nAlignment = 0;
	this->m_nBufferSize = 0;
	this->m_nFramesInFlight = 0;
}

void GPURingBuffer::Init(uint32_t nBufferSize, uint32_t nAlignment, uint32_t nFramesInFlight) {
	this->m_nBufferSize = nBufferSize;
	this->m_nAlignment = nAlignment;
	this->m_nFramesInFlight = nFramesInFlight;
}

void* GPURingBuffer::Allocate(uint32_t nDataSize, uint64_t& outOffset) { return nullptr; }

void GPURingBuffer::Reset() { }

uint64_t GPURingBuffer::GetSize() { return 0; }