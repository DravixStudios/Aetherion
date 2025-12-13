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

void Transform::Translate(Vector3 v) {
	this->Translate(v.x, v.y, v.z);
}

void Transform::Rotate(float x, float y, float z) {
	this->rotation.x += x;
	this->rotation.y += y;
	this->rotation.z += z;
}

void Transform::Rotate(float v) {
	this->Rotate(v, v, v);
}

void Transform::Rotate(Vector3 v) {
	this->Rotate(v.x, v.y, v.z);
}

Vector3 Transform::Forward() {
	Vector3 v = Vector3{ 0.f, 0.f, 1.f };
	return this->RotatePoint(v);
}

Vector3 Transform::Right() {
	Vector3 v = Vector3{ 1.f, 0.f, 0.f };
	return this->RotatePoint(v);
}

Vector3 Transform::RotatePoint(Vector3& point) {
	glm::mat4 rot = glm::mat4(1.f);
	rot = glm::rotate(rot, glm::radians(this->rotation.y), { 0.f, 1.f, 0.f });
	rot = glm::rotate(rot, glm::radians(this->rotation.x), { 1.f, 0.f, 0.f });
	rot = glm::rotate(rot, glm::radians(this->rotation.z), { 0.f, 0.f, 1.f });

	glm::vec4 rotToPoint = rot * glm::vec4(point.x, point.y, point.z, 0.f);
	return { rotToPoint.x, rotToPoint.y, rotToPoint.z };
}