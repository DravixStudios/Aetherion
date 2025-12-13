#include "Core/Input.h"

Input* Input::m_instance;

Input::Input() {
	this->deltaX = 0.f;
	this->deltaY = 0.f;
	this->centerX = 0.f;
	this->centerY = 0.f;
}

void Input::ShowCursor(bool bShow) {
	if (!bShow) {
		glfwSetInputMode(this->m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else {

	}
}

void Input::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
}

Input* Input::GetInstance() {
	if (Input::m_instance == nullptr)
		Input::m_instance = new Input();

	return Input::m_instance;
}