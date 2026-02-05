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

enum class EAccess : uint32_t {
	NONE = 0,
	INDIRECT_COMMAND_READ = 1,
	INDEX_READ = 1 << 1,
	VERTEX_ATTRIBUTE_READ = 1 << 2,
	UNIFORM_READ = 1 << 3,
	INPUT_ATTACHMENT_READ = 1 << 4,
	SHADER_READ = 1 << 5,
	SHADER_WRITE = 1 << 6,
	COLOR_ATTACHMENT_READ = 1 << 7,
	COLOR_ATTACHMENT_WRITE = 1 << 8,
	DEPTH_STENCIL_READ = 1 << 9,
	DEPTH_STENCIL_WRITE =  1 << 10,
	TRANSFER_READ = 1 << 11,
	TRANSFER_WRITE = 1 << 12,
	HOST_READ = 1 << 13,
	HOST_WRITE = 1 << 14,
	MEMORY_READ = 1 << 15,
	MEMORY_WRITE =  1 << 16
};

inline EAccess
operator|(EAccess a, EAccess b) {
	return static_cast<EAccess>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EAccess
operator&(EAccess a, EAccess b) {
	return static_cast<EAccess>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

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