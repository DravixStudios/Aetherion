#include "Math/Transform.h"

Transform::Transform() {
	this->location = Vector3{};
	this->rotation = Vector3{};
	this->scale = Vector3{  1.f, 1.f, 1.f };
}