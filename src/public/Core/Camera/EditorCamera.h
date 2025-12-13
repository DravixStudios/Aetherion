#pragma once
#include "Camera.h"
#include "Core/Input.h"

class EditorCamera : public Camera {
public:
	EditorCamera(std::string name);

	void Start() override;
	void Update() override;

private:
	float m_yaw;
	float m_pitch;
	Input* m_input;
};