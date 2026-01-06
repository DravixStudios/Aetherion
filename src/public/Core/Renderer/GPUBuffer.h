#pragma once
#include <iostream>

enum EBufferType {
	VERTEX_BUFFER = 0x00,
	STAGING_BUFFER = 0x01,
	CONSTANT_BUFFER = 0x02,
	INDEX_BUFFER = 0x03,
	STORAGE_BUFFER = 0x04,
	UNKNOWN_BUFFER = 0xFF
};

class GPUBuffer {
	friend class Renderer;
	friend class VulkanRenderer;
public:
	GPUBuffer(EBufferType bufferType = EBufferType::VERTEX_BUFFER) : m_bufferType(bufferType) {};
	virtual ~GPUBuffer() {};

	virtual uint32_t GetSize() = 0;
	EBufferType GetBufferType() const { return this->m_bufferType; }
protected:
	EBufferType m_bufferType;
};