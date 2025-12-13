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
		/* Disable cursor */
		glfwSetInputMode(this->m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		/* Get window size */
		int width, height = 0;
		glfwGetWindowSize(this->m_pWindow, &width, &height);

		/* Calculate center position */
		this->centerX = static_cast<float>(width / 2);
		this->centerY = static_cast<float>(height / 2);

		/* Get actual cursor position */
		float posX, posY = 0.f;
		glfwGetCursorPos(this->m_pWindow, &posX, &posY);

		/* Calculate delta */
		this->deltaX = this->centerX - posX;
		this->deltaY = this->centerY - posY;

		/* Move cursor to the center */
		glfwSetCursorPos(this->m_pWindow, this->centerX, this->centerY);
	}
	else {
		glfwSetInputMode(this->m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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