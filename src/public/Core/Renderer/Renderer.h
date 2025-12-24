#pragma once
#include <iostream>


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Utils.h"
#include "Core/Renderer/GPUBuffer.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/GPUFormat.h"

#include "Core/Containers.h"

enum ECubemapLayout {
    HORIZONTAL_CROSS, // 4x3
    VERTICAL_CROSS, // 3x4
    HORIZONTAL_STRIP, // 6x1
    VERTICAL_STRIP // 1x6
};

class Renderer {
protected:
    GLFWwindow* m_pWindow;
public:
    Renderer();
    virtual void Init();
    virtual void Update();
    virtual void SetWindow(GLFWwindow* pWindow);

    virtual GPUBuffer* CreateBuffer(const void* pData, uint32_t nSize, EBufferType bufferType);

    template<typename T>
    GPUBuffer* CreateVertexBuffer(const Vector<T>& vertices) {
        return this->CreateVertexBufferRaw(
            vertices.data(),
            static_cast<uint32_t>(vertices.size()),
            sizeof(T)
        );
    }

protected:
    virtual GPUBuffer* CreateVertexBufferRaw(const void* pData, uint32_t nCount, uint32_t nStride);

public:
    virtual GPUBuffer* CreateIndexBuffer(const Vector<uint16_t>& indices);
    virtual GPUBuffer* CreateStagingBuffer(void* pData, uint32_t nSize);
    virtual GPUTexture* CreateTexture(
        GPUBuffer* pBuffer, 
        uint32_t nWidth,
        uint32_t nHeight,
        GPUFormat format
    );
    virtual GPUTexture* CreateCubemap(const String filePath, ECubemapLayout layout = HORIZONTAL_CROSS);
    virtual void ExtractCubemapFaces(
        const float* pcSrcRGBA, 
        int nSrcWidth, 
        int nSrcHeight, 
        float* pDstData, 
        int nFaceWidth, 
        int nFaceHeight, 
        ECubemapLayout layout
    );
    virtual bool DrawVertexBuffer(GPUBuffer* buffer);
    virtual bool DrawIndexBuffer(GPUBuffer* vbo, GPUBuffer* ibo);
};
