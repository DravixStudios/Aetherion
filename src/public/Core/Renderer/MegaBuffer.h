#pragma once
#include "Utils.h"
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUBuffer.h"

struct MegaBufferAllocation {
	uint32_t nVertexOffset;
	uint32_t nFirstIndex;
	uint32_t nIndexCount;
};

class MegaBuffer {
public:
	void Init(Ref<Device> device, uint32_t nMaxVertices, uint32_t nMaxIndices);
	MegaBufferAllocation Upload(const Vector<Vertex>& vertices, const Vector<uint16_t>& indices);

	Ref<GPUBuffer> GetVertexBuffer() const { return this->m_vertexBuffer; }
	Ref<GPUBuffer> GetIndexBuffer() const { return this->m_indexBuffer; }
private:
	Ref<Device> m_device;
	Ref<GPUBuffer> m_vertexBuffer;
	Ref<GPUBuffer> m_indexBuffer;

	uint32_t m_nCurrentVertexOffset = 0;
	uint32_t m_nCurrentIndexOffset = 0;

	void* m_pVertexMap = nullptr;
	void* m_pIndexMap = nullptr;
};