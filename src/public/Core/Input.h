#pragma once
#include <iostream>
#include <map>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class Input {
public:
	Input();

	void ShowCursor(bool bShow);
	void SetWindow(GLFWwindow* pWindow);

private:
	float deltaX;
	float deltaY;
	float centerX;
	float centerY;

	GLFWwindow* m_pWindow;

	static Input* GetInstance();

	static Input* m_instance;
};