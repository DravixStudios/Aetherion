#include "Core/Renderer/Renderer.h"

Renderer::Renderer() {
	this->m_pWindow = nullptr;
}

void Renderer::Init() {}
void Renderer::Update() {}

GPUBuffer* Renderer::CreateBuffer(const void* pData, uint32_t nSize, EBufferType bufferType) { return nullptr; }

GPUBuffer* Renderer::CreateVertexBuffer(const std::vector<Vertex>& vertices) { return nullptr; }

GPUBuffer* Renderer::CreateStagingBuffer(void* pData, uint32_t nSize) { return nullptr; }

GPUTexture* Renderer::CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) { return nullptr; }

bool Renderer::DrawVertexBuffer(GPUBuffer* buffer) { return false; }

void Renderer::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
	return;
}