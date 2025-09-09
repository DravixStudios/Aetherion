#include "Core/Renderer/Renderer.h"

Renderer::Renderer() {
	this->m_pWindow = nullptr;
}

void Renderer::Init() {}
void Renderer::Update() {}

void Renderer::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
	return;
}