#include "Core/Camera/Camera.h"

Camera::Camera(std::string name) {
	this->m_name = name;
	this->transform.location = { 0.f, 0.f, 0.f };
	this->transform.rotation = { 0.f, 0.f, 0.f };
	this->transform.scale = { 1.f, 1.f, 1.f };
	this->m_time = Time::GetInstance();
}

void Camera::Start() {

}

void Camera::Update() {

}