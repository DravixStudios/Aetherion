#include "Core/Renderer/MegaBuffer.h"

/**
* Mega Buffer initialization
* 
* @param device Logical device
* @param nMaxVertices Max vertex count
* @param nMaxIndices Max index count
*/
void
MegaBuffer::Init(Ref<Device> device, uint32_t nMaxVertices, uint32_t nMaxIndices) {
	this->m_device = device;

	/* Create vertex buffer */
	BufferCreateInfo vboInfo = { };
	vboInfo.nSize = nMaxVertices * sizeof(Vertex);
	vboInfo.usage = EBufferUsage::VERTEX_BUFFER | EBufferUsage::TRANSFER_DST;
	vboInfo.type = EBufferType::VERTEX_BUFFER;
	vboInfo.sharingMode = ESharingMode::EXCLUSIVE;

	this->m_vertexBuffer = this->m_device->CreateBuffer(vboInfo);

	/* Create index buffer */
	BufferCreateInfo iboInfo = { };
	iboInfo.nSize = nMaxIndices * sizeof(uint16_t);
	iboInfo.usage = EBufferUsage::INDEX_BUFFER | EBufferUsage::TRANSFER_DST;
	iboInfo.type = EBufferType::INDEX_BUFFER;
	iboInfo.sharingMode = ESharingMode::EXCLUSIVE;
	
	this->m_indexBuffer = this->m_device->CreateBuffer(iboInfo);

	/* Map buffers */
	this->m_pVertexMap = this->m_vertexBuffer->Map();
	this->m_pIndexMap = this->m_indexBuffer->Map();
}

/**
* Upload to the Mega buffer
* 
* @param vertices Vertices
* @param indices Indices
* 
* @returns Mega buffer allocation data
*/
MegaBufferAllocation
MegaBuffer::Upload(const Vector<Vertex>& vertices, const Vector<uint16_t>& indices) {
	MegaBufferAllocation alloc = { };
	alloc.nVertexOffset = this->m_nCurrentVertexOffset;
	alloc.nFirstIndex = this->m_nCurrentIndexOffset;
	alloc.nIndexCount = static_cast<uint32_t>(indices.size());

	size_t vertexSize = vertices.size() * sizeof(Vertex);
	size_t indexSize = indices.size() * sizeof(uint16_t);

	memcpy(
		static_cast<char*>(this->m_pVertexMap) + (this->m_nCurrentVertexOffset * sizeof(Vertex)),
		vertices.data(), 
		vertexSize
	);

	memcpy(
		static_cast<char*>(this->m_pVertexMap) + (this->m_nCurrentIndexOffset * sizeof(uint16_t)),
		indices.data(),
		indexSize
	);

	return alloc;
}