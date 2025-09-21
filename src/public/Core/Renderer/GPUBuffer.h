#pragma once
#include <iostream>

class GPUBuffer {
public:
	GPUBuffer() {};
	~GPUBuffer() {};

	virtual uint32_t GetSize() = 0;
};