#include "Core/Renderer/Rendering/Passes/CullingPass.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"

/**
* Culling pass initialization
* 
* @param device Logical device
*/
void 
CullingPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Culling pass initialization
*
* @param device Logical device
* @param nFramesInFlight Frames in flight count
*/
void
CullingPass::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;
	
	this->CreateResources();
	this->CreateDescriptors();
	this->CreatePipeline();
}

/**
* Culling pass node setup
* 
* @param builder Render graph builder
*/
void 
CullingPass::SetupNode(RenderGraphBuilder& builder) {
	builder.SetComputeOnly();
}

/**
* Executes the culling pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
* @param nFrameIndex Current frame index (default = 0)
*/
void
CullingPass::Execute(
	Ref<GraphicsContext> context,
	RenderGraphContext& graphCtx,
	uint32_t nFrameIndex
) {
	Ref<DescriptorSet> cullingSet = this->m_cullingSets[nFrameIndex];

	context->FillBuffer(this->m_countBuffer, 0, sizeof(uint32_t), 0);
	context->BufferMemoryBarrier(this->m_countBuffer, EAccess::TRANSFER_WRITE, EAccess::SHADER_WRITE);

	context->BindPipeline(this->m_computePipeline);
	context->BindDescriptorSets(0, { this->m_cullingSets[nFrameIndex]});
	
	uint32_t nGroups = (this->m_nTotalBatches + 255) / 256;

	glm::mat4 viewProj = this->m_viewProj;
	glm::vec4 frustumPlanes[6];
	this->ExtractFrustumPlanes(viewProj, frustumPlanes);

	struct {
		glm::mat4 viewProj;
		glm::vec4 frustumPlanes[6];
		uint32_t nTotalBatches;
		uint32_t nWvpAlignment;
	} pushData;

	pushData.viewProj = viewProj;
	pushData.nTotalBatches = this->m_nTotalBatches;
	pushData.nWvpAlignment = this->m_wvpBuffer->GetAlignment();

	memcpy(pushData.frustumPlanes, frustumPlanes, sizeof(frustumPlanes));

	context->PushConstants(this->m_pipelineLayout, EShaderStage::COMPUTE, 0, sizeof(pushData), &pushData);

	context->Dispatch(nGroups, 1, 1);

	context->BufferMemoryBarrier(
	this->m_indirectBuffer->GetBuffer(),
		EAccess::SHADER_WRITE, 
		EAccess::INDIRECT_COMMAND_READ
	);
	context->BufferMemoryBarrier(this->m_countBuffer, EAccess::SHADER_WRITE, EAccess::INDIRECT_COMMAND_READ);
}

void
CullingPass::CreatePipeline() {
	/* Compile shader */
	Ref<Shader> computeShader = Shader::CreateShared();
	computeShader->LoadFromGLSL("GPUCulling.comp", EShaderStage::COMPUTE);

	/* Push constants */
	PushConstantRange pushRange = { };
	pushRange.stage = EShaderStage::COMPUTE;
	pushRange.nOffset = 0;
	pushRange.nSize = sizeof(glm::mat4) + (sizeof(glm::vec4) * 6) + (sizeof(uint32_t) * 2);

	ComputePipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shader = computeShader;
	pipelineInfo.descriptorSetLayouts = { this->m_setLayout };
	pipelineInfo.pushConstantRanges = { pushRange };

	this->m_computePipeline = this->m_device->CreateComputePipeline(pipelineInfo);
	this->m_pipelineLayout = this->m_computePipeline->GetLayout();
}

/**
* Creates culling pass resources
*/
void 
CullingPass::CreateResources() {
	constexpr uint32_t MAX_OBJECT = 131072; // Max objects (2^17)
	constexpr uint32_t MAX_BATCHES = 131072; // Max objects (2^17)
	constexpr uint32_t MAX_DRAWS = 131072; // Max objects (2^17)

	/* 
		ObjectInstanceData ring buffer 
		
		Alignment: 16 (4 uint32_t)
	*/
	RingBufferCreateInfo instanceInfo = { };
	instanceInfo.nAlignment = 16;
	instanceInfo.nFramesInFlight = this->m_nFramesInFlight;
	instanceInfo.nBufferSize = MAX_OBJECT * sizeof(ObjectInstanceData);
	instanceInfo.usage = EBufferUsage::STORAGE_BUFFER;

	this->m_instanceBuffer = this->m_device->CreateRingBuffer(instanceInfo);

	/* DrawIndexedIndirectCommand ring buffer */
	RingBufferCreateInfo indirectInfo = { };
	indirectInfo.nAlignment = 32;
	indirectInfo.nFramesInFlight = this->m_nFramesInFlight;
	indirectInfo.nBufferSize = MAX_DRAWS * sizeof(DrawIndexedIndirectCommand);
	indirectInfo.usage = EBufferUsage::STORAGE_BUFFER | EBufferUsage::INDIRECT_BUFFER;

	this->m_indirectBuffer = this->m_device->CreateRingBuffer(indirectInfo);

	/* WVP Ring buffer */
	RingBufferCreateInfo wvpInfo = { };
	wvpInfo.nAlignment = 256;
	wvpInfo.nBufferSize = 2 * 1024 * 1024;
	wvpInfo.nFramesInFlight = this->m_nFramesInFlight;
	wvpInfo.usage = EBufferUsage::STORAGE_BUFFER;

	this->m_wvpBuffer = this->m_device->CreateRingBuffer(wvpInfo);

	/* DrawBatch ring buffer */
	RingBufferCreateInfo batchInfo = { };
	batchInfo.nAlignment = 16;
	batchInfo.nBufferSize = sizeof(DrawBatch) * MAX_BATCHES;
	batchInfo.nFramesInFlight = this->m_nFramesInFlight;
	batchInfo.usage = EBufferUsage::STORAGE_BUFFER;

	this->m_batchBuffer = this->m_device->CreateRingBuffer(batchInfo);

	/* Draw count buffer (4 bytes) */
	BufferCreateInfo countInfo = { };
	countInfo.sharingMode = ESharingMode::EXCLUSIVE;
	countInfo.nSize = sizeof(uint32_t);
	countInfo.usage = EBufferUsage::STORAGE_BUFFER | EBufferUsage::TRANSFER_DST | EBufferUsage::INDIRECT_BUFFER;
	countInfo.type = EBufferType::STORAGE_BUFFER;

	this->m_countBuffer = this->m_device->CreateBuffer(countInfo);

	/* Initialize FrameIndirectData */
	this->m_frameData.resize(this->m_nFramesInFlight);
	for (FrameIndirectData& frameData : this->m_frameData) {
		frameData.instanceDataOffset = 0;
		frameData.batchDataOffset = 0;
		frameData.indirectDrawOffset = 0;
		frameData.objectCount = 0;
		frameData.wvpOffset = 0;
	}

	Logger::Debug("CullingPass::CreateResources: Culling pass resources created");
}

/**
* Culling pass descriptor creation
*/
void
CullingPass::CreateDescriptors() {
	Vector<DescriptorSetLayoutBinding> bindings(5);
	
	/* Binding 0: Instance data (ObjectInstanceData) */
	bindings[0].nBinding = 0;
	bindings[0].nDescriptorCount = 1;
	bindings[0].stageFlags = EShaderStage::COMPUTE | EShaderStage::VERTEX;
	bindings[0].descriptorType = EDescriptorType::STORAGE_BUFFER;

	/* Binding 1: Draw batches (DrawBatch) */
	bindings[1].nBinding = 1;
	bindings[1].nDescriptorCount = 1;
	bindings[1].stageFlags = EShaderStage::COMPUTE;
	bindings[1].descriptorType = EDescriptorType::STORAGE_BUFFER;

	/* Binding 2: Output commands (DrawIndexedIndirectCommand) */
	bindings[2].nBinding = 2;
	bindings[2].nDescriptorCount = 1;
	bindings[2].stageFlags = EShaderStage::COMPUTE;
	bindings[2].descriptorType = EDescriptorType::STORAGE_BUFFER;

	/* Binding 3: Draw count */
	bindings[3].nBinding = 3;
	bindings[3].nDescriptorCount = 1;
	bindings[3].stageFlags = EShaderStage::COMPUTE;
	bindings[3].descriptorType = EDescriptorType::STORAGE_BUFFER;

	/* Binding 4: WVP Ring buffer (read only) */
	bindings[4].nBinding = 4;
	bindings[4].nDescriptorCount = 1;
	bindings[4].stageFlags = EShaderStage::COMPUTE | EShaderStage::VERTEX;
	bindings[4].descriptorType = EDescriptorType::STORAGE_BUFFER;

	/* Create descriptor set layout */
	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = bindings;
	
	this->m_setLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	Logger::Debug("CullingPass::CreateDescriptors: Culling pass descriptor set layout created");

	/* Create descriptor pool */
	DescriptorPoolSize poolSize = { };
	poolSize.type = EDescriptorType::STORAGE_BUFFER;
	poolSize.nDescriptorCount = 5 * this->m_nFramesInFlight;

	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = this->m_nFramesInFlight;
	poolInfo.poolSizes = { poolSize };

	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Allocate descriptor sets */
	this->m_cullingSets.resize(this->m_nFramesInFlight);

	for (uint32_t i = 0; i < this->m_nFramesInFlight; i++) {
		this->m_cullingSets[i] = this->m_device->CreateDescriptorSet(this->m_pool, this->m_setLayout);
	}

	/* Writes descriptor sets */
	Vector<DescriptorBufferInfo> bufferInfos(5);

	/* Binding 0: Instance data */
	bufferInfos[0].buffer = this->m_instanceBuffer->GetBuffer();
	bufferInfos[0].nOffset = 0;
	bufferInfos[0].nRange = 0;

	/* Binding 1: Batch data */
	bufferInfos[1].buffer = this->m_batchBuffer->GetBuffer();
	bufferInfos[1].nOffset = 0;
	bufferInfos[1].nRange = 0;

	/* Binding 2: Output commands */
	bufferInfos[2].buffer = this->m_indirectBuffer->GetBuffer();
	bufferInfos[2].nOffset = 0;
	bufferInfos[2].nRange = 0;

	/* Binding 3: Draw count */
	bufferInfos[3].buffer = this->m_countBuffer;
	bufferInfos[3].nOffset = 0;
	bufferInfos[3].nRange = 0;

	/* Binding 4: WVP Buffer */
	bufferInfos[4].buffer = this->m_wvpBuffer->GetBuffer();
	bufferInfos[4].nOffset = 0;
	bufferInfos[4].nRange = 0;

	for (uint32_t i = 0; i < this->m_nFramesInFlight; i++) {
		Ref<DescriptorSet> set = this->m_cullingSets[i];
		
		for (uint32_t j = 0; j < 5; j++) {
			set->WriteBuffer(j, 0, bufferInfos[j]);
		}
		set->UpdateWrites();
	}
}

void 
CullingPass::ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 planes[6]) {
	/* Left plane */
	planes[0].x = viewProj[0][3] + viewProj[0][0];
	planes[0].y = viewProj[1][3] + viewProj[1][0];
	planes[0].z = viewProj[2][3] + viewProj[2][0];
	planes[0].w = viewProj[3][3] + viewProj[3][0];

	/* Right plane */
	planes[1].x = viewProj[0][3] - viewProj[0][0];
	planes[1].y = viewProj[1][3] - viewProj[1][0];
	planes[1].z = viewProj[2][3] - viewProj[2][0];
	planes[1].w = viewProj[3][3] - viewProj[3][0];

	/* Bottom plane */
	planes[2].x = viewProj[0][3] + viewProj[0][1];
	planes[2].y = viewProj[1][3] + viewProj[1][1];
	planes[2].z = viewProj[2][3] + viewProj[2][1];
	planes[2].w = viewProj[3][3] + viewProj[3][1];

	/* Top plane */
	planes[3].x = viewProj[0][3] - viewProj[0][1];
	planes[3].y = viewProj[1][3] - viewProj[1][1];
	planes[3].z = viewProj[2][3] - viewProj[2][1];
	planes[3].w = viewProj[3][3] - viewProj[3][1];

	/* Near plane */
	planes[4].x = viewProj[0][3] + viewProj[0][2];
	planes[4].y = viewProj[1][3] + viewProj[1][2];
	planes[4].z = viewProj[2][3] + viewProj[2][2];
	planes[4].w = viewProj[3][3] + viewProj[3][2];

	/* Far plane */
	planes[5].x = viewProj[0][3] - viewProj[0][2];
	planes[5].y = viewProj[1][3] - viewProj[1][2];
	planes[5].z = viewProj[2][3] - viewProj[2][2];
	planes[5].w = viewProj[3][3] - viewProj[3][2];

	/* Normalize planes */
	for (uint32_t i = 0; i < 6; i++) {
		float length = glm::length(glm::vec3(planes[i]));
		planes[i] /= length;
	}
}		   