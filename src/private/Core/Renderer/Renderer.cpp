#include "Core/Renderer/Renderer.h"

Renderer::Renderer() {
	this->m_pWindow = nullptr;
}

void Renderer::Init() {}
void Renderer::Update() {}

GPUBuffer* Renderer::CreateVertexBuffer(const std::vector<Vertex>& vertices) { return nullptr; }

bool Renderer::DrawVertexBuffer(GPUBuffer* buffer) { return false; }

void Renderer::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
	return;
}