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
	
	this->m_nInitialMaxVertices = nMaxVertices;
	this->m_nInitialMaxIndices = nMaxIndices;

	Block block = this->CreateBlock(nMaxVertices, nMaxIndices);
	this->m_blocks.push_back(block);
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
	uint32_t nVertexCount = static_cast<uint32_t>(vertices.size());
	uint32_t nIndexCount = static_cast<uint32_t>(indices.size());

	/* Get the last block */
	Block& currentBlock = this->m_blocks.back();

	bool bVertexFits = (currentBlock.nCurrentVertexOffset + nVertexCount) <= currentBlock.nMaxVertices;
	bool bIndexFits = (currentBlock.nCurrentIndexOffset + nIndexCount) <= currentBlock.nMaxIndices;

	/* If it doesn't fit, create a new block with the double of capacity */
	if (!bVertexFits || !bIndexFits) {
		uint32_t nNewMaxVertices = currentBlock.nMaxVertices * 2;
		uint32_t nNewMaxIndices = currentBlock.nMaxIndices * 2;

		/* Assert new block can store the mesh */
		nNewMaxVertices = std::max(nNewMaxVertices, nVertexCount);
		nNewMaxIndices = std::max(nNewMaxIndices, nIndexCount);

		this->m_blocks.push_back(this->CreateBlock(nNewMaxVertices, nNewMaxIndices));
	}

	/* Write on the new block (it can be a new block) */
	Block& targetBlock = this->m_blocks.back();
	uint32_t nBlockIdx = static_cast<uint32_t>(this->m_blocks.size() - 1);

	MegaBufferAllocation alloc = { };
	alloc.nBlockIndex = nBlockIdx;
	alloc.nVertexOffset = targetBlock.nCurrentVertexOffset;
	alloc.nFirstIndex = targetBlock.nCurrentIndexOffset;
	alloc.nIndexCount = nIndexCount;

	memcpy(
		static_cast<char*>(targetBlock.pVertexMap) + (targetBlock.nCurrentVertexOffset * sizeof(Vertex)),
		vertices.data(), 
		nVertexCount * sizeof(Vertex)
	);

	memcpy(
		static_cast<char*>(targetBlock.pIndexMap) + (targetBlock.nCurrentIndexOffset * sizeof(uint32_t)),
		indices.data(),
		nIndexCount * sizeof(uint32_t)
	);

	targetBlock.nCurrentVertexOffset += nVertexCount;
	targetBlock.nCurrentIndexOffset += nIndexCount;

	return alloc;
}

MegaBuffer::Block
MegaBuffer::CreateBlock(uint32_t nMaxVertices, uint32_t nMaxIndices) {
	Block block = { };
	block.nMaxVertices = nMaxVertices,
	block.nMaxIndices = nMaxIndices;

	/* Create VBO */
	BufferCreateInfo vboInfo = { };
	vboInfo.nSize = nMaxVertices * sizeof(Vertex);
	vboInfo.type = EBufferType::VERTEX_BUFFER;
	vboInfo.usage = EBufferUsage::VERTEX_BUFFER;
	vboInfo.sharingMode = ESharingMode::EXCLUSIVE;
	
	block.vertexBuffer = this->m_device->CreateBuffer(vboInfo);

	/* Create IBO */
	BufferCreateInfo iboInfo = { };
	iboInfo.nSize = nMaxIndices * sizeof(uint32_t);
	iboInfo.type = EBufferType::INDEX_BUFFER;
	iboInfo.usage = EBufferUsage::INDEX_BUFFER;
	iboInfo.sharingMode = ESharingMode::EXCLUSIVE;

	block.indexBuffer = this->m_device->CreateBuffer(iboInfo);

	/* Map memory */
	block.pVertexMap = block.vertexBuffer->Map();
	block.pIndexMap = block.indexBuffer->Map();

	return block;
}