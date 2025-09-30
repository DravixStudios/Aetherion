#pragma once
#include <iostream>

class GPUTexture {
public:
	GPUTexture() {};
	~GPUTexture() {};

	virtual uint32_t GetSize() = 0;
};