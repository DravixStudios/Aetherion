#include "Core/Camera/Camera.h"

Camera::Camera(String name) {
	this->m_name = name;
	this->transform.location = { 0.f, 0.f, 0.f };
	this->transform.rotation = { 0.f, 0.f, 0.f };
	this->transform.scale = { 1.f, 1.f, 1.f };
	this->m_time = Time::GetInstance();
}

void 
Camera::Start() {

}

void
Camera::Update() {

}

/**
* Resizes the camera 
* 
* @param nWidth New width
* @param nHeight New height
*/
void 
Camera::Resize(uint32_t nWidth, uint32_t nHeight) {
	if (nWidth <= 0 || nHeight <= 0) {
		Logger::Error("Camera::Resize: Width or height can't be 0");
		return;
	}

	this->m_nWidth = nWidth;
	this->m_nHeight = nHeight;
}