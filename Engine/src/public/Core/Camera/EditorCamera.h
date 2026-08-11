#pragma once
#include "Camera.h"
#include "Core/Input.h"

class EditorCamera : public Camera {
public:
	EditorCamera(String name);

	void Start() override;
	void Update() override;

private:
	float m_yaw;
	float m_pitch;

	float m_moveSpeed;
	float m_sensX;
	float m_sensY;

	Input* m_input;
};