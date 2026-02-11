#include "Core/Renderer/Vulkan/VulkanGraphicsContext.h"
#include "Core/Renderer/Vulkan/VulkanTexture.h"

VulkanGraphicsContext::VulkanGraphicsContext(Ref<VulkanCommandBuffer> commandBuffer) 
	: m_commandBuffer(commandBuffer), m_currentPipeline(VK_NULL_HANDLE), 
	m_currentBindPoint(VK_PIPELINE_BIND_POINT_MAX_ENUM), m_currentPipelineLayout(VK_NULL_HANDLE) {}

/**
* Binds a Vulkan pipeline
* 
* @param pipeline Binded pipeline
*/
void 
VulkanGraphicsContext::BindPipeline(Ref<Pipeline> pipeline) {
	Ref<VulkanPipeline> vkPipeline = pipeline.As<VulkanPipeline>();
	this->m_currentPipeline = vkPipeline->GetVkPipeline();
	this->m_currentBindPoint = vkPipeline->GetVkBindPoint();
	this->m_currentPipelineLayout = vkPipeline->GetVkPipelineLayout();

	vkCmdBindPipeline(this->m_commandBuffer->GetVkCommandBuffer(), this->m_currentBindPoint, this->m_currentPipeline);
}

/**
* Binds descriptor sets
* 
* @param nFirstSet
* @param sets DescriptorSet vector
* @param dynamicOffsets  Dynamic offsets (optional)
*/
void 
VulkanGraphicsContext::BindDescriptorSets(
	uint32_t nFirstSet,
	const Vector<Ref<DescriptorSet>>& sets, 
	const Vector<uint32_t>& dynamicOffsets
) {
	uint32_t nSetCount = sets.size();

	Vector<VkDescriptorSet> vkDescriptorSets;
	vkDescriptorSets.resize(nSetCount);

	for (uint32_t i = 0; i < nSetCount; i++) {
		const Ref<DescriptorSet>& descriptorSet = sets[i];
		vkDescriptorSets[i] = descriptorSet.As<VulkanDescriptorSet>()->GetVkSet();
	}

	vkCmdBindDescriptorSets(
		this->m_commandBuffer->GetVkCommandBuffer(),
		this->m_currentBindPoint, 
		this->m_currentPipelineLayout, 
		nFirstSet, 
		nSetCount, 
		vkDescriptorSets.data(), 
		dynamicOffsets.size(), dynamicOffsets.data()
	);
}

/**
* Binds vertex buffers
* 
* @param buffers Vertex buffer vector
* @param offsets Offsets (default = {})
*/
void 
VulkanGraphicsContext::BindVertexBuffers(
	const Vector<Ref<GPUBuffer>>& buffers, 
	const Vector<size_t>& offsets
) {
	uint32_t nBufferCount = buffers.size();

	Vector<VkBuffer> vkBuffers;
	vkBuffers.resize(nBufferCount);

	for (uint32_t i = 0; i < nBufferCount; i++) {
		const Ref<GPUBuffer>& buffer = buffers[i];
		vkBuffers[i] = buffer.As<VulkanBuffer>()->GetVkBuffer();
	}

	Vector<VkDeviceSize> vkOffsets;
	if (offsets.empty()) {
		vkOffsets.resize(nBufferCount, 0);
	} else {
		vkOffsets.resize(offsets.size());
		std::transform(
			offsets.begin(),
			offsets.end(),
			vkOffsets.begin(),
			[](size_t offset) { return static_cast<VkDeviceSize>(offset); }
		);
	}

	vkCmdBindVertexBuffers(
		this->m_commandBuffer->GetVkCommandBuffer(), 
		0,
		nBufferCount, vkBuffers.data(), 
		vkOffsets.data()
	);
}

/**
* Binds a index buffer
* 
* @param buffer Index buffer
* @param indexType Index type (default = UINT16)
*/
void 
VulkanGraphicsContext::BindIndexBuffer(Ref<GPUBuffer> buffer, EIndexType indexType) {
	VkIndexType vkIndexType = VulkanHelpers::ConvertIndexType(indexType);

	VkBuffer vkBuffer = buffer.As<VulkanBuffer>()->GetVkBuffer();
	vkCmdBindIndexBuffer(this->m_commandBuffer->GetVkCommandBuffer(), vkBuffer, 0, vkIndexType);
}

/**
* Performs a draw call
*
* @param nVertexCount Number of vertices
* @param nInstanceCount Number of instances (default = 1)
* @param nFirstVertex First vertex (default = 0)
* @param nFirstInstance First instance (default = 0)
*/
void 
VulkanGraphicsContext::Draw(
	uint32_t nVertexCount,
	uint32_t nInstanceCount,
	uint32_t nFirstVertex,
	uint32_t nFirstInstance
) {
	vkCmdDraw(
		this->m_commandBuffer->GetVkCommandBuffer(),
		nVertexCount, 
		nInstanceCount, 
		nFirstVertex, 
		nFirstInstance
	);
}


/**
* Performs a indexed draw call
*
* @param nIndexCount Number of indices
* @param nInstanceCount Number of instances (default = 1)
* @param nFirstIndex First index (default = 0)
* @param nVertexOffset Vertex offset (default = 0)
* @param nFirstInstance First instance (default = 0)
*/
void 
VulkanGraphicsContext::DrawIndexed(
	uint32_t nIndexCount,
	uint32_t nInstanceCount,
	uint32_t nFirstIndex,
	uint32_t nVertexOffset,
	uint32_t nFirstInstance
) {
	vkCmdDrawIndexed(
		this->m_commandBuffer->GetVkCommandBuffer(),
		nIndexCount,
		nInstanceCount,
		nFirstIndex,
		nVertexOffset,
		nFirstInstance
	);
}

/**
* Performs an indirect indexed draw call
*
* @param buffer Buffer containing draw parameters
* @param nOffset Byte offset into buffer where parameters begin
* @param countBuffer Buffer containing draw count
* @param nCountBufferOffset Byte offset into countBuffer where the draw count begins
* @param nMaxDrawCount Maximum number of draws that will be executed
* @param nStride Byte stride between successive sets of draw parameters.
*/
void 
VulkanGraphicsContext::DrawIndexedIndirect(
	Ref<GPUBuffer> buffer,
	uint32_t nOffset,
	Ref<GPUBuffer> countBuffer,
	uint32_t nCountBufferOffset,
	uint32_t nMaxDrawCount,
	uint32_t nStride
) {
	Ref<VulkanDevice> device = this->m_commandBuffer->GetDevice();
	VkBuffer vkBuffer = buffer.As<VulkanBuffer>()->GetVkBuffer();

	/* 
		If draw indirect count is supported, use  
		vkCmdDrawIndexedIndirectCount, if it is
		not, we'll use a "simulation" of what it
		does, but CPU-side.
	*/
	if (device->IsExtensionSupported("VK_KHR_draw_indirect_count")) {
	
		VkBuffer vkCountBuffer = countBuffer.As<VulkanBuffer>()->GetVkBuffer();

		vkCmdDrawIndexedIndirectCount(
			this->m_commandBuffer->GetVkCommandBuffer(),
			vkBuffer,
			nOffset,
			vkCountBuffer,
			nCountBufferOffset,
			nMaxDrawCount,
			nStride
		);
	}
	else {
		uint32_t nDrawCount = 0;

		void* pMap = countBuffer->Map();
		nDrawCount = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pMap) + nCountBufferOffset);
		countBuffer->Unmap();
		pMap = nullptr;

		nDrawCount = std::min(nDrawCount, nMaxDrawCount);

		vkCmdDrawIndexedIndirect(
			this->m_commandBuffer->GetVkCommandBuffer(),
			vkBuffer,
			nCountBufferOffset,
			nDrawCount,
			nStride
		);
	}

}

/**
* Push constants to pipeline layout
*
* @param layout Pipeline layout
* @param stages Shader stage
* @param nOffset Push constant offset
* @param nSize Size of push constant data
* @param pcData Constant pointer to push constant data
*/
void 
VulkanGraphicsContext::PushConstants(
	Ref<PipelineLayout> layout,
	EShaderStage stages,
	uint32_t nOffsets,
	uint32_t nSize,
	const void* pcData
) {
	VkPipelineLayout vkLayout = layout.As<VulkanPipelineLayout>()->GetVkLayout();

	vkCmdPushConstants(
		this->m_commandBuffer->GetVkCommandBuffer(),
		vkLayout,
		VulkanHelpers::ConvertShaderStage(stages),
		nOffsets,
		nSize,
		pcData
	);
}

/**
* Set viewport
*
* @param viewport Viewport struct
*/
void VulkanGraphicsContext::SetViewport(const Viewport& viewport) {
	VkViewport vkViewport = { };
	vkViewport.x = viewport.x;
	vkViewport.y = viewport.y;
	vkViewport.width = viewport.width;
	vkViewport.height = viewport.height;
	vkViewport.minDepth = viewport.minDepth;
	vkViewport.maxDepth = viewport.maxDepth;
	
	vkCmdSetViewport(this->m_commandBuffer->GetVkCommandBuffer(), 0, 1, &vkViewport);
}

/**
* Set scissor
*
* @param scissor Scissor rect
*/
void VulkanGraphicsContext::SetScissor(const Rect2D& scissor) {
	VkExtent2D extent = { };
	extent.width = scissor.extent.width;
	extent.height = scissor.extent.height;

	VkOffset2D offset = { };
	offset.x = scissor.offset.x;
	offset.y = scissor.offset.y;

	VkRect2D vkScissor = { offset, extent };

	vkCmdSetScissor(this->m_commandBuffer->GetVkCommandBuffer(), 0, 1, &vkScissor);
}

/**
* Begins a Vulkan render pass
*
* @param beginInfo Render pass begin info
*/
void 
VulkanGraphicsContext::BeginRenderPass(const RenderPassBeginInfo& beginInfo) {
	VkRenderPass rp = beginInfo.renderPass.As<VulkanRenderPass>()->GetVkRenderPass();
	VkFramebuffer fb = beginInfo.framebuffer.As<VulkanFramebuffer>()->GetVkFramebuffer();

	VkRenderPassBeginInfo rpInfo = { };
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass = rp;
	rpInfo.framebuffer = fb;
	rpInfo.renderArea.extent = {
		beginInfo.renderArea.extent.width,
		beginInfo.renderArea.extent.height
	};
	rpInfo.renderArea.offset = {
		static_cast<int32_t>(beginInfo.renderArea.offset.x),
		static_cast<int32_t>(beginInfo.renderArea.offset.y)
	};

	Vector<VkClearValue> clearValues;
	clearValues.reserve(beginInfo.clearValues.size());

	for (const ClearValue& clear : beginInfo.clearValues) {
		VkClearValue vkClear = { };
		if (clear.type == ClearValue::Type::COLOR) {
			vkClear.color = { { clear.color.r, clear.color.g, clear.color.b, clear.color.a } };
		}
		else {
			vkClear.depthStencil = { clear.deptStencil.depth, clear.deptStencil.stencil };
		}
		clearValues.push_back(vkClear);
	}

	rpInfo.clearValueCount = clearValues.size();
	rpInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(this->m_commandBuffer->GetVkCommandBuffer(), &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

/**
* Ends the current Vulkan render pass
*/
void
VulkanGraphicsContext::EndRenderPass() {
	vkCmdEndRenderPass(this->m_commandBuffer->GetVkCommandBuffer());
}

/**
* Advances to the next Vulkan subpass
*/
void 
VulkanGraphicsContext::NextSubpass() {
	vkCmdNextSubpass(this->m_commandBuffer->GetVkCommandBuffer(), VK_SUBPASS_CONTENTS_INLINE);
}

/**
* Fills a buffer
*
* @param buffer Buffer to fill
* @param nOffset Offset
* @param nSize Size of the fill
* @param nData Fill data
*/
void 
VulkanGraphicsContext::FillBuffer(Ref<GPUBuffer> buffer, uint32_t nOffset, uint32_t nSize, uint32_t nData) {
	VkBuffer vkBuffer = buffer.As<VulkanBuffer>()->GetVkBuffer();

	vkCmdFillBuffer(this->m_commandBuffer->GetVkCommandBuffer(), vkBuffer, nOffset, nSize, nData);
}

/**
* Dispatches compute work items
*
* @param x X dimension
* @param y Y dimension
* @param z Z dimension
*/
void 
VulkanGraphicsContext::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
	vkCmdDispatch(this->m_commandBuffer->GetVkCommandBuffer(), x, y, z);
}

/**
* Buffer memory barrier
*
* @param buffer Buffer
* @param srcAccess Source access mask
* @param dstAccess Destination access mask
*/
void 
VulkanGraphicsContext::BufferMemoryBarrier(Ref<GPUBuffer> buffer, EAccess srcAccess, EAccess dstAccess) {
	VkBuffer vkBuffer = buffer.As<VulkanBuffer>()->GetVkBuffer();

	VkBufferMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	
	barrier.srcAccessMask = VulkanHelpers::ConvertAccess(srcAccess);
	barrier.dstAccessMask = VulkanHelpers::ConvertAccess(dstAccess);

	barrier.buffer = vkBuffer;

	barrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(
		this->m_commandBuffer->GetVkCommandBuffer(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0, 
		0, nullptr, 
		1, &barrier,
		0, nullptr
	);
}

/**
* Image memory barrier for layout transitions
*
* @param image Image to transition
* @param oldLayout Old layout
* @param newLayout New layout
*/
void 
VulkanGraphicsContext::ImageBarrier(
	Ref<GPUTexture> image,
	EImageLayout oldLayout,
	EImageLayout newLayout
) {
	this->ImageBarrier(image, oldLayout, newLayout, 1, 0, 0);
}

void 
VulkanGraphicsContext::ImageBarrier(
	Ref<GPUTexture> image,
	EImageLayout oldLayout,
	EImageLayout newLayout,
	uint32_t nLayerCount,
	uint32_t nBaseMipLevel,
	uint32_t nBaseArrayLayer
) {
	VkImage vkImage = image.As<VulkanTexture>()->GetVkImage();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VulkanHelpers::ConvertImageLayout(oldLayout);
	barrier.newLayout = VulkanHelpers::ConvertImageLayout(newLayout);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = nBaseMipLevel;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = nBaseArrayLayer;
	barrier.subresourceRange.layerCount = nLayerCount;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	/* Infer access masks and pipeline stages from layouts */
	if (oldLayout == EImageLayout::UNDEFINED) {
		barrier.srcAccessMask = 0;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	} else if (oldLayout == EImageLayout::COLOR_ATTACHMENT) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else if (oldLayout == EImageLayout::TRANSFER_DST) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	if (newLayout == EImageLayout::SHADER_READ_ONLY) {
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (newLayout == EImageLayout::COLOR_ATTACHMENT) {
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else if (newLayout == EImageLayout::TRANSFER_DST) {
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}

	vkCmdPipelineBarrier(
		this->m_commandBuffer->GetVkCommandBuffer(),
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

/**
* Inserts a global memory barrier to synchronize all operations.
* This is a simple but expensive synchronization method.
*/
void 
VulkanGraphicsContext::GlobalBarrier() {
	VkMemoryBarrier barrier = { };
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
		VK_ACCESS_INDEX_READ_BIT |
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
		VK_ACCESS_UNIFORM_READ_BIT |
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
		VK_ACCESS_SHADER_READ_BIT |
		VK_ACCESS_SHADER_WRITE_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_TRANSFER_READ_BIT |
		VK_ACCESS_TRANSFER_WRITE_BIT |
		VK_ACCESS_HOST_READ_BIT |
		VK_ACCESS_HOST_WRITE_BIT;

	barrier.dstAccessMask = barrier.srcAccessMask;

	vkCmdPipelineBarrier(
		this->m_commandBuffer->GetVkCommandBuffer(),
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		0,
		1, &barrier,
		0, nullptr,
		0, nullptr
	);
}

