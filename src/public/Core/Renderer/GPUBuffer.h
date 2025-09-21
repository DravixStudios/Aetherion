#pragma once
#include <iostream>

class GPUBuffer {
public:
	GPUBuffer() {};
	~GPUBuffer() {};

	virtual size_t GetSize() const = 0;
};