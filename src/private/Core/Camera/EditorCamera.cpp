#include "Core/Camera/EditorCamera.h"

EditorCamera::EditorCamera(std::string name) : Camera::Camera(name) {
	this->m_input = Input::GetInstance();
	this->m_pitch = 0.f;
	this->m_yaw = 0.f;
}

void EditorCamera::Start() {
	Camera::Start();
}

void EditorCamera::Update() {
	Camera::Update();

	if (this->m_input->GetKeyDown('W')) {
		this->transform.Translate(this->transform.Forward() * 0.001f);
	}

	if (this->m_input->GetKeyDown('S')) {
		this->transform.Translate(this->transform.Forward() * -0.001f);
	}

	if (this->m_input->GetKeyDown('D')) {
		this->transform.Translate(this->transform.Right() * 0.001f);
	}

	if (this->m_input->GetKeyDown('A')) {
		this->transform.Translate(this->transform.Right() *  -0.001f);
	}

	if (this->m_input->GetKeyDown('E')) {
		this->transform.Translate(0.f, 0.001f, 0.f);
	}

	if (this->m_input->GetKeyDown('Q')) {
		this->transform.Translate(0.f, -0.001f, 0.f);
	}

	if (this->m_input->GetButtonDown(EMouseButton::RIGHT)) {
		this->m_input->ShowCursor(false);

		float deltaX = this->m_input->GetDeltaX();
		float deltaY = this->m_input->GetDeltaY();;

		float newPitch = m_pitch + deltaY * -0.01f;
		newPitch = glm::clamp(newPitch, -89.f, 89.f);
		float deltaPitchClamped = newPitch - this->m_pitch;
		this->m_pitch = newPitch;

		//this->m_pitch = glm::clamp(this->m_pitch, glm::radians(-89.0f), glm::radians(89.0f));

		this->transform.Rotate(deltaY * -0.01f, deltaX * -0.01f, 0.f);
	}
	else {
		this->m_input->ShowCursor(true);
	}
}