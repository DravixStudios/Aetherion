#pragma once
#include <iostream>

#include "Core/Containers.h"

enum class EBufferType {
	VERTEX_BUFFER,
	STAGING_BUFFER,
	CONSTANT_BUFFER,
	INDEX_BUFFER ,
	STORAGE_BUFFER,
	UNKNOWN_BUFFER 
};

enum class EBufferUsage : uint32_t {
	NONE = 0,
	TRANSFER_SRC = 1,
	TRANSFER_DST = 1 << 1,
	VERTEX_BUFFER = 1 << 2,
	INDEX_BUFFER = 1 << 3,
	UNIFORM_BUFFER = 1 << 4,
	STORAGE_BUFFER = 1 << 5,
	INDIRECT_BUFFER = 1 << 6
};

inline EBufferUsage
operator|(EBufferUsage a, EBufferUsage b) {
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EBufferUsage
operator&(EBufferUsage a, EBufferUsage b) {
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class EIndexType {
	UINT8,
	UINT16,
	UINT32
};

class GPUBuffer {
public:
	static constexpr const char* CLASS_NAME = "GPUBuffer";

	using Ptr = Ref<GPUBuffer>;

	virtual ~GPUBuffer() {};
	
	virtual void Create(const void* pcData, uint32_t nSize, EBufferType bufferType = EBufferType::VERTEX_BUFFER) = 0;

	EBufferType GetBufferType() const { return this->m_bufferType; }
protected:
	EBufferType m_bufferType = EBufferType::UNKNOWN_BUFFER;
};