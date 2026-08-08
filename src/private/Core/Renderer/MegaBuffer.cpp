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
MegaBuffer::Upload(const Vector<Vertex>& vertices, const Vector<uint32_t>& indices) {
	uint32_t nVertexCount = static_cast<uint32_t>(vertices.size());
	uint32_t nIndexCount = static_cast<uint32_t>(indices.size());
	
	/* Check if fits inside of a free memory allocation */
	Vector<Block>& blocks = this->m_blocks;
	for (uint32_t nBlockIdx = 0; nBlockIdx < blocks.size(); nBlockIdx++) {
		Block& block = blocks[nBlockIdx];

		if (block.freeVertices.empty() || block.freeIndices.empty())
			continue;

		/* Get vertex iterator for free segment */
		Vector<FreeSegment>::const_iterator vertexIter = std::find_if(
			block.freeVertices.begin(),
			block.freeVertices.end(), 
			[nVertexCount](const FreeSegment& segment) {
				return segment.nCount >= nVertexCount;
			}
		);

		if (vertexIter == block.freeVertices.end())
			continue;

		/* Get index iterator for free segment */
		Vector<FreeSegment>::const_iterator indexIter = std::find_if(
			block.freeIndices.begin(),
			block.freeIndices.end(),
			[nIndexCount](const FreeSegment& segment) {
				return segment.nCount >= nIndexCount;
			}
		);

		if (indexIter == block.freeIndices.end())
			continue;

		/* Calculate vertices and indices index */
		const uint32_t nVertexSegmentIdx = vertexIter - block.freeVertices.begin();
		const uint32_t nIndexSegmentIdx = indexIter - block.freeIndices.begin();

		FreeSegment vertexSegment = block.freeVertices[nVertexSegmentIdx];
		FreeSegment indexSegment = block.freeIndices[nIndexSegmentIdx];
	
		/* Create staging vertex and index buffers */
		BufferCreateInfo bufferInfo = { };
		bufferInfo.pcData = vertices.data();
		bufferInfo.nSize = nVertexCount * sizeof(Vertex);
		bufferInfo.sharingMode = ESharingMode::EXCLUSIVE;
		bufferInfo.type = EBufferType::STAGING_BUFFER;
		bufferInfo.usage = EBufferUsage::TRANSFER_SRC;

		Ref<GPUBuffer> stagingVertex = this->m_device->CreateBuffer(bufferInfo);

		bufferInfo.pcData = indices.data();
		bufferInfo.nSize = nIndexCount * sizeof(uint32_t);

		Ref<GPUBuffer> stagingIndex = this->m_device->CreateBuffer(bufferInfo);

		/* Copy staging buffers */
		block.vertexBuffer->CopyBuffer(
			stagingVertex,
			nVertexCount * sizeof(Vertex), 
			vertexSegment.nCount * sizeof(Vertex)
		);

		block.indexBuffer->CopyBuffer(
			stagingIndex,
			nIndexCount * sizeof(uint32_t),
			indexSegment.nCount * sizeof(uint32_t)
		);

		/* TODO: Modify segments to don't lose all of the segment space */

		/* Create a megabuffer allocation */
		MegaBufferAllocation alloc = { };
		alloc.nBlockIndex = nBlockIdx;
		alloc.nVertexOffset = block.nCurrentVertexOffset;
		alloc.nFirstIndex = block.nCurrentIndexOffset;
		alloc.nIndexCount = nIndexCount;

		return alloc;
	}

	/* Get the last block */
	Block& currentBlock = this->m_blocks.back();

	const bool bVertexFits = (currentBlock.nCurrentVertexOffset + nVertexCount) <= currentBlock.nMaxVertices;
	const bool bIndexFits = (currentBlock.nCurrentIndexOffset + nIndexCount) <= currentBlock.nMaxIndices;

	/* If it doesn't fit, create a new block with the double of capacity */
	if (!bVertexFits || !bIndexFits) {
		uint32_t nNewMaxVertices = currentBlock.nMaxVertices * 2;
		uint32_t nNewMaxIndices = currentBlock.nMaxIndices * 2;

		/* Assert new block can store the mesh */
		nNewMaxVertices = std::max(nNewMaxVertices, nVertexCount);
		nNewMaxIndices = std::max(nNewMaxIndices, nIndexCount);

		this->m_blocks.push_back(this->CreateBlock(nNewMaxVertices, nNewMaxIndices));
	}

	/* Create index and vertex staging buffers */
	BufferCreateInfo bufferInfo = { };
	bufferInfo.pcData = vertices.data();
	bufferInfo.nSize = nVertexCount * sizeof(Vertex);
	bufferInfo.sharingMode = ESharingMode::EXCLUSIVE;
	bufferInfo.type = EBufferType::STAGING_BUFFER;
	bufferInfo.usage = EBufferUsage::TRANSFER_SRC;
	
	Ref<GPUBuffer> stagingVertex = this->m_device->CreateBuffer(bufferInfo);

	bufferInfo.pcData = indices.data();
	bufferInfo.nSize = nIndexCount * sizeof(uint32_t);

	Ref<GPUBuffer> stagingIndex = this->m_device->CreateBuffer(bufferInfo);

	/* Write on the block (it can be a new block) */
	Block& targetBlock = this->m_blocks.back();
	uint32_t nBlockIdx = static_cast<uint32_t>(this->m_blocks.size() - 1);

	MegaBufferAllocation alloc = { };
	alloc.nBlockIndex = nBlockIdx;
	alloc.nVertexOffset = targetBlock.nCurrentVertexOffset;
	alloc.nFirstIndex = targetBlock.nCurrentIndexOffset;
	alloc.nIndexCount = nIndexCount;

	targetBlock.vertexBuffer->CopyBuffer(
		stagingVertex,
		nVertexCount * sizeof(Vertex), 
		targetBlock.nCurrentVertexOffset * sizeof(Vertex)
	);

	targetBlock.indexBuffer->CopyBuffer(
		stagingIndex, 
		nIndexCount * sizeof(uint32_t), 
		targetBlock.nCurrentIndexOffset * sizeof(uint32_t)
	);

	targetBlock.nCurrentVertexOffset += nVertexCount;
	targetBlock.nCurrentIndexOffset += nIndexCount;

	stagingVertex = Ref<GPUBuffer>();
	stagingIndex = Ref<GPUBuffer>();

	return alloc;
}

void 
MegaBuffer::Free(const MegaBufferAllocation& alloc) {
	if (alloc.nBlockIndex >= this->m_blocks.size()) return;

	Block& block = this->m_blocks[alloc.nBlockIndex];

	block.freeVertices.push_back({ alloc.nVertexOffset, alloc.nIndexCount });
	block.freeIndices.push_back({ alloc.nFirstIndex, alloc.nIndexCount });
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
	vboInfo.usage = EBufferUsage::VERTEX_BUFFER | EBufferUsage::TRANSFER_DST;
	vboInfo.sharingMode = ESharingMode::EXCLUSIVE;
	
	block.vertexBuffer = this->m_device->CreateBuffer(vboInfo);

	/* Create IBO */
	BufferCreateInfo iboInfo = { };
	iboInfo.nSize = nMaxIndices * sizeof(uint32_t);
	iboInfo.type = EBufferType::INDEX_BUFFER;
	iboInfo.usage = EBufferUsage::INDEX_BUFFER | EBufferUsage::TRANSFER_DST;
	iboInfo.sharingMode = ESharingMode::EXCLUSIVE;

	block.indexBuffer = this->m_device->CreateBuffer(iboInfo);

	return block;
}