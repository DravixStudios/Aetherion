#include "Core/Input.h"

Input* Input::m_instance;

Input::Input() {
	this->deltaX = 0.f;
	this->deltaY = 0.f;
	this->centerX = 0.f;
	this->centerY = 0.f;
	this->m_pWindow = nullptr;
}

void Input::ShowCursor(bool bShow) {
	if (!bShow) {
		/* Disable cursor */
		glfwSetInputMode(this->m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		/* Get window size */
		int nWidth = 0; 
		int nHeight = 0;;
		glfwGetWindowSize(this->m_pWindow, &nWidth, &nHeight);

		/* Calculate center position */
		this->centerX = static_cast<float>(nWidth / 2);
		this->centerY = static_cast<float>(nHeight / 2);

		/* Get actual cursor position */
		double posX, posY = 0.f;
		glfwGetCursorPos(this->m_pWindow, &posX, &posY);

		/* Calculate delta */
		this->deltaX = this->centerX - static_cast<float>(posX);
		this->deltaY = this->centerY - static_cast<float>(posY);

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

void Input::SetKey(char key, EInputState state) {
	this->m_keys[key] = state;
}

void Input::SetKeyDown(char key) {
	this->SetKey(key, EInputState::PRESSED);
}

void Input::SetKeyUp(char key) {
	this->SetKey(key, EInputState::RELEASED);
}

void Input::SetButton(EMouseButton btn, EInputState state) {
	this->m_buttons[btn] = state;
}

void Input::SetButtonDown(EMouseButton btn) {
	this->SetButton(btn, EInputState::PRESSED);
}

void Input::SetButtonUp(EMouseButton btn) {
	this->SetButton(btn, EInputState::RELEASED);
}

bool Input::GetKey(char key, EInputState state) {
	if (this->m_keys.count(key) < 1)
		return false;

	if (this->m_keys[key] == state)
		return true;

	return false;
}

bool Input::GetKeyDown(char key) {
	return this->GetKey(key, EInputState::PRESSED);
}

bool Input::GetKeyUp(char key) {
	return this->GetKey(key, EInputState::RELEASED);
}

bool Input::GetButton(EMouseButton btn, EInputState state) {
	if (this->m_buttons.count(btn) < 1)
		return false;

	if (this->m_buttons[btn] == state)
		return true;

	return false;
}

bool Input::GetButtonDown(EMouseButton btn) {
	return this->GetButton(btn, EInputState::PRESSED);
}

bool Input::GetButtonUp(EMouseButton btn) {
	return this->GetButton(btn, EInputState::RELEASED);
}

float Input::GetDeltaX() {
	return this->deltaX;
}

float Input::GetDeltaY() {
	return this->deltaY;
}

void Input::Callback(EInputType nEventType, int nKeyOrButton, int nAction, float posX, float posY) {
	switch (nEventType) {
	case EInputType::KEYBOARD:
		if (nAction == GLFW_PRESS) 
			SetKeyDown(static_cast<char>(nKeyOrButton));

		if (nAction == GLFW_RELEASE) 
			SetKeyUp(static_cast<char>(nKeyOrButton));

		break;
	case EInputType::MOUSE_BUTTON:
		if (nKeyOrButton == GLFW_MOUSE_BUTTON_LEFT) {
			if (nAction == GLFW_PRESS) 
				this->SetButtonDown(EMouseButton::LEFT);

			if (nAction == GLFW_RELEASE) 
				this->SetButtonUp(EMouseButton::LEFT);
		}

		if (nKeyOrButton == GLFW_MOUSE_BUTTON_RIGHT) {
			if (nAction == GLFW_PRESS)
				this->SetButtonDown(EMouseButton::RIGHT);

			if (nAction == GLFW_RELEASE)
				this->SetButtonUp(EMouseButton::RIGHT);
		}
		break;
	}
}

void Input::KeyCallback(GLFWwindow* pWindow, int nKey, int nScancode, int nAction, int nMods) {
	Input::GetInstance()->Callback(EInputType::KEYBOARD, nKey, nAction);
}

void Input::MouseButtonCallback(GLFWwindow* pWindow, int nButton, int nAction, int nMods) {
	Input::GetInstance()->Callback(EInputType::MOUSE_BUTTON, nButton, nAction);
}

void Input::Close() {
	Vector<char> keysToRemove;
	for (std::pair<char, EInputState> key : this->m_keys) {
		if (key.second == EInputState::RELEASED)
			keysToRemove.push_back(key.first);
	}

	Vector<EMouseButton> buttonsToRemove;
	for (std::pair<EMouseButton, EInputState> button : this->m_buttons) {
		if (button.second == EInputState::RELEASED)
			buttonsToRemove.push_back(button.first);
	}

	for (char key : keysToRemove)
		this->m_keys.erase(key);

	for (EMouseButton button : buttonsToRemove)
		this->m_buttons.erase(button);

	this->deltaX = 0.f;
	this->deltaY = 0.f;
}

Input* Input::GetInstance() {
	if (Input::m_instance == nullptr)
		Input::m_instance = new Input();

	return Input::m_instance;
}