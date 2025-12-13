#include "Math/Transform.h"

Transform::Transform() {
	this->location = Vector3{};
	this->rotation = Vector3{};
	this->scale = Vector3{  1.f, 1.f, 1.f };
}

void Transform::Translate(float x, float y, float z) {
	this->location.x += x;
	this->location.y += y;
	this->location.z += z;
}

void Transform::Translate(float v) {
	this->Translate(v, v, v);
}

void Transform::Translate(Vector3& v) {
	this->Translate(v.x, v.y, v.z);
}