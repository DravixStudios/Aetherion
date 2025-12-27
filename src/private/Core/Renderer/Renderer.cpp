#include "Core/Renderer/Renderer.h"

Renderer::Renderer() {
	this->m_pWindow = nullptr;
}

void Renderer::Init() {}
void Renderer::Update() {}

GPUBuffer* Renderer::CreateBuffer(const void* pData, uint32_t nSize, EBufferType bufferType) { return nullptr; }

GPUBuffer* Renderer::CreateVertexBufferRaw(const void* pData, uint32_t nCount, uint32_t nStride) { 
	return this->CreateBuffer(pData, nCount * nStride, EBufferType::VERTEX_BUFFER);
}

GPUBuffer* Renderer::CreateIndexBuffer(const Vector<uint16_t>& indices) { return nullptr; }

GPUBuffer* Renderer::CreateStagingBuffer(void* pData, uint32_t nSize) { return nullptr; }

GPUTexture* Renderer::CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format) { return nullptr; }

GPUTexture* Renderer::CreateCubemap(const String filePath, ECubemapLayout layout) { return nullptr; }

void Renderer::ConvertEquirectangularToHorizontalCross(
    const float* pcSrcRGBA,
    int nSrcWidth,
    int nSrcHeight,
    float* pDstData,
    int nFaceWidth,
    int nFaceSize
) { }

void Renderer::ExtractCubemapFaces(
    const float* pcSrcRGBA,
    int nSrcWidth,
    int nSrcHeight,
    float* pDstData,
    int nFaceWidth,
    int nFaceHeight,
    ECubemapLayout layout
) {}

bool Renderer::DrawVertexBuffer(GPUBuffer* buffer) { return false; }

bool Renderer::DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo) { return false; }

void Renderer::SetWindow(GLFWwindow* pWindow) {
	this->m_pWindow = pWindow;
	return;
}