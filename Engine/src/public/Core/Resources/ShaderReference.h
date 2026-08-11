#pragma once
#include "Core/Containers.h"

struct ShaderReference {
	Path shaderPath;

	bool operator!() const {
		return this->shaderPath.Length() <= 0;
	}
};