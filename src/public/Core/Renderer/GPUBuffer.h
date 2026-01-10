#pragma once
#include <iostream>

enum EBufferType {
	VERTEX_BUFFER,
	STAGING_BUFFER,
	CONSTANT_BUFFER,
	INDEX_BUFFER ,
	STORAGE_BUFFER,
	UNKNOWN_BUFFER 
};

enum class EIndexType {
	UINT8,
	UINT16,
	UINT32
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