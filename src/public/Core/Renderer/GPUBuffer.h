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
};