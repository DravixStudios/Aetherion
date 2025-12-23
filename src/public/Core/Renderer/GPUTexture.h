#pragma once
#include <iostream>

enum ETextureType {
	TEXTURE = 0,
	CUBEMAP = 1
};

class GPUTexture {
public:
	GPUTexture(ETextureType textureType = ETextureType::TEXTURE) { this->textureType = textureType; }
	~GPUTexture() {}

	virtual uint32_t GetSize() = 0;
	
	ETextureType textureType;
};