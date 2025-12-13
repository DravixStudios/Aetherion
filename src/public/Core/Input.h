#pragma once
#include <iostream>
#include <map>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

enum EInputState {
	PRESSED = 0,
	RELEASED = 1
};

enum EMouseButton {
	LEFT = 0,
	RIGHT = 1
};

enum EInputType {
	KEYBOARD = 0,
	MOUSE_BUTTON = 1
};

class Input {
public:
	Input();

	void ShowCursor(bool bShow);
	void SetWindow(GLFWwindow* pWindow);
	
	/* Key setters */
	void SetKey(char key, EInputState state);
	void SetKeyDown(char key);
	void SetKeyUp(char key);

	/* Button setters */
	void SetButton(EMouseButton btn, EInputState state);
	void SetButtonDown(EMouseButton btn);
	void SetButtonUp(EMouseButton btn);

	/* Key getters */
	bool GetKey(char key, EInputState state);
	bool GetKeyDown(char key);
	bool GetKeyUp(char key);

	/* Button Getters */
	bool GetButton(EMouseButton btn, EInputState state);
	bool GetButtonDown(EMouseButton btn);
	bool GetButtonUp(EMouseButton btn);

	/* GLFW Callbacks */
	void Callback(EInputType nEventType, int nKeyOrButton = 0, int nAction = 0, float posX = 0.f, float posY = 0.f);
	static void KeyCallback(GLFWwindow* pWindow, int nKey, int nScancode, int nAction, int nMods);
	static void MouseButtonCallback(GLFWwindow* pWindow, int nButton, int nAction, int nMods);

	void Close();

	static Input* GetInstance();
private:
	float deltaX;
	float deltaY;
	float centerX;
	float centerY;

	GLFWwindow* m_pWindow;

	std::map<char, EInputState> m_keys;
	std::map<EMouseButton, EInputState> m_buttons;


	static Input* m_instance;
};