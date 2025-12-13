#pragma once
#include <iostream>
#include "Math/Vector3.h"

struct Transform {
	Vector3 location;
	Vector3 rotation;
	Vector3 scale;

	Transform();

	void Translate(float x, float y, float z);
	void Translate(float v);
	void Translate(Vector3& v);
};