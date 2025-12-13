#include "Core/Camera/EditorCamera.h"

EditorCamera::EditorCamera(std::string name) : Camera::Camera(name) {
	this->m_input = Input::GetInstance();
}

void EditorCamera::Start() {
	Camera::Start();
}

void EditorCamera::Update() {
	Camera::Update();

	if (this->m_input->GetKeyDown('W')) {
		this->transform.Translate(0.f, 0.f, 0.01f);
	}
}