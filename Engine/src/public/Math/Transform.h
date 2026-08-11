#pragma once
#include <iostream>
#include "Math/Vector3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

struct Transform {
	Vector3 location;
	Vector3 rotation;
	Vector3 scale;

	Transform();

	void Translate(float x, float y, float z);
	void Translate(float v);
	void Translate(Vector3 v);

	void Rotate(float x, float y, float z);
	void Rotate(float v);
	void Rotate(Vector3 v);

	Vector3 Forward();
	Vector3 Right();

	Vector3 RotatePoint(Vector3& point);

	glm::mat4 GetWorldMatrix() const;
};