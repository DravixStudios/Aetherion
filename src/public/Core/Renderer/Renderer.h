#pragma once
#include <iostream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utils.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUFormat.h"

class Renderer {
protected:
    GLFWwindow* m_pWindow;
public:
    Renderer();
    virtual void Init();
    virtual void Update();
    virtual void SetWindow(GLFWwindow* pWindow);

    virtual GPUBuffer* CreateVertexBuffer(const std::vector<Vertex>& vertices);
    virtual GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize);
    virtual GPUTexture* CreateTexture(GPUBuffer* pBuffer, uint32_t nWidth, uint32_t nHeight, GPUFormat format);
    virtual bool DrawVertexBuffer(GPUBuffer* buffer);
};
