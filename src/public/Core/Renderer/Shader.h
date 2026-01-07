#pragma once
#include <iostream>
#include "Core/Containers.h"

enum EShaderStage {
	VERTEX = 0x01,
	FRAGMENT = 0x02,
	COMPUTE = 0x03,
	GEOMETRY = 0x04,
	TESSELATION_CONTROL = 0x05,
	TESSELATION_EVALUATION = 0x06
};

class Shader {
public:
	Shader() = default;
	virtual ~Shader() = default;
	virtual void LoadFromFile(const String& path, EShaderStage shaderStage) = 0;
	virtual void LoadFromSource(const String& source, EShaderStage shaderStage) = 0;
};