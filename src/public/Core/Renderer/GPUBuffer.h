#pragma once
#include <iostream>

enum EBufferType {
	VERTEX_BUFFER = 0x00,
	STAGING_BUFFER = 0x01,
	CONSTANT_BUFFER = 0x02,
	INDEX_BUFFER = 0x03,
	UNKNOWN_BUFFER = 0xFF
};

class GPUBuffer {
public:
	GPUBuffer() {};
	~GPUBuffer() {};

	virtual uint32_t GetSize() = 0;
protected:
	EBufferType m_bufferType;
};