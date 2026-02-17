#include "Core/Camera/EditorCamera.h"
#include "Core/Core.h"

EditorCamera::EditorCamera(String name) : Camera::Camera(name) {
	this->m_input = Input::GetInstance();
	this->m_pitch = 0.f;
	this->m_yaw = 0.f;
	this->m_moveSpeed = 10.f;
	this->m_sensX = .2f;
	this->m_sensY = .1f;
}

void EditorCamera::Start() {
	Camera::Start();
}

void EditorCamera::Update() {
	Camera::Update();
	
	if (this->m_input->GetButtonDown(EMouseButton::RIGHT)) {
		this->m_input->ShowCursor(false);

		if (this->m_input->GetKeyDown('W')) {
			this->transform.Translate(this->transform.Forward() * this->m_time->deltaTime * this->m_moveSpeed);
		}

		if (this->m_input->GetKeyDown('S')) {
			this->transform.Translate(this->transform.Forward() * this->m_time->deltaTime * -this->m_moveSpeed);
		}

		if (this->m_input->GetKeyDown('D')) {
			this->transform.Translate(this->transform.Right() * this->m_time->deltaTime * this->m_moveSpeed);
		}

		if (this->m_input->GetKeyDown('A')) {
			this->transform.Translate(this->transform.Right() * this->m_time->deltaTime * -this->m_moveSpeed);
		}

		if (this->m_input->GetKeyDown('E')) {
			this->transform.Translate(0.f, this->m_time->deltaTime * 5.f, 0.f);
		}

		if (this->m_input->GetKeyDown('Q')) {
			this->transform.Translate(0.f, this->m_time->deltaTime * -5.f, 0.f);
		}

		float deltaX = this->m_input->GetDeltaX();
		float deltaY = this->m_input->GetDeltaY();;

		float newPitch = m_pitch + deltaY * -0.01f;
		newPitch = glm::clamp(newPitch, -89.f, 89.f);
		float deltaPitchClamped = newPitch - this->m_pitch;
		this->m_pitch = newPitch;

		//this->m_pitch = glm::clamp(this->m_pitch, glm::radians(-89.0f), glm::radians(89.0f));

		this->transform.Rotate(deltaY * -this->m_sensY, deltaX * -this->m_sensX, 0.f);

		glm::vec3 pos(this->transform.location.x, this->transform.location.y, this->transform.location.z);
		glm::vec3 forward = glm::normalize(
			glm::vec3(
				this->transform.Forward().x, 
				this->transform.Forward().y,
				this->transform.Forward().x
			)
		);

		this->m_view = glm::mat4(1.f);
		this->m_view = glm::rotate(this->m_view, glm::radians(this->transform.rotation.x), glm::vec3(1.f, 0.f, 0.f));
		this->m_view = glm::rotate(this->m_view, glm::radians(this->transform.rotation.y), glm::vec3(0.f, 1.f, 0.f));
		this->m_view = glm::translate(this->m_view, {
			-this->transform.location.x,
			-this->transform.location.y,
			this->transform.location.z
		});

		this->m_projection = glm::perspectiveFovRH(
			glm::radians(70.f), 
			static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 
			1.f, 300.f
		);
	}
	else {
		this->m_input->ShowCursor(true);
	}
}