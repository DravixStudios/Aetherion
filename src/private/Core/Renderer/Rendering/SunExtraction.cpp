#include "Core/Renderer/Rendering/SunExtraction.h"

/**
* Sun extraction initialization
* 
* @param device Logical device
* @param skybox Skybox image
* @param skyboxView Skybox image view
* @param cubeSampler Cubemap sampler
*/
void
SunExtraction::Init(Ref<Device> device, Ref<GPUTexture> skybox, Ref<ImageView> skyboxView, Ref<Sampler> cubeSampler) {
	this->m_device = device;
	this->m_skybox = skybox;
	this->m_skyboxView = skyboxView;
	this->m_sampler = cubeSampler;

	/* Create sun result buffer */
	BufferCreateInfo bufferInfo = { };
	bufferInfo.nSize = sizeof(glm::vec4);
	bufferInfo.sharingMode = ESharingMode::EXCLUSIVE;
	bufferInfo.type = EBufferType::STORAGE_BUFFER;
	bufferInfo.usage = EBufferUsage::STORAGE_BUFFER;
	
	this->m_sunResultBuff = this->m_device->CreateBuffer(bufferInfo);

	this->CreateDescriptors();
	this->CreatePipeline();
}

/**
* Extract sun position and
* stores it into a buffer
*/
void 
SunExtraction::Extract(Ref<GraphicsContext> context) {
	context->GlobalBarrier();
	context->BindPipeline(this->m_pipeline);

	context->BindDescriptorSets(0, { this->m_descriptorSet });
	context->Dispatch(1, 1, 1);
}

/**
* Reads the sun result buffer
* and converts it into a glm::vec4
* 
* @returns Sun result vector
*/
glm::vec4
SunExtraction::ReadSunResult() {
	if (!this->m_bBufferRead) {

		this->m_bBufferRead = true;
	}
	void* pMap = this->m_sunResultBuff->Map();
	glm::vec4 sunResult = *static_cast<glm::vec4*>(pMap);
	this->m_sunResultBuff->Unmap();

	if (glm::length(glm::vec3(sunResult)) > 0.001f) {
		this->m_sunResult = sunResult;
	}


	return this->m_sunResult;
}

/**
* Creates sun extraction descriptors
*/
void
SunExtraction::CreateDescriptors() {
	/* Create descriptor set layout */
	DescriptorSetLayoutBinding skyboxBinding = { };
	skyboxBinding.nBinding = 0;
	skyboxBinding.descriptorType = EDescriptorType::COMBINED_IMAGE_SAMPLER;
	skyboxBinding.nDescriptorCount = 1;
	skyboxBinding.stageFlags = EShaderStage::COMPUTE;

	DescriptorSetLayoutBinding sunBinding = { };
	sunBinding.nBinding = 1;
	sunBinding.descriptorType = EDescriptorType::STORAGE_BUFFER;
	sunBinding.nDescriptorCount = 1;
	sunBinding.stageFlags = EShaderStage::COMPUTE;

	DescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.bindings = Vector{ skyboxBinding, sunBinding };
	
	this->m_setLayout = this->m_device->CreateDescriptorSetLayout(layoutInfo);

	/* Create descriptor pool */
	DescriptorPoolSize samplerPoolSize = { };
	samplerPoolSize.nDescriptorCount = 1;
	samplerPoolSize.type = EDescriptorType::COMBINED_IMAGE_SAMPLER;

	DescriptorPoolSize bufferPoolSize = { };
	bufferPoolSize.nDescriptorCount = 1;
	bufferPoolSize.type = EDescriptorType::STORAGE_BUFFER;
	
	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = 1;
	poolInfo.poolSizes = Vector{ samplerPoolSize, bufferPoolSize };
	
	this->m_pool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Allocate descriptor set */
	this->m_descriptorSet = this->m_device->CreateDescriptorSet(this->m_pool, this->m_setLayout);

	/* Write descriptor set */
	DescriptorImageInfo samplerInfo = { };
	samplerInfo.texture = this->m_skybox;
	samplerInfo.imageView = this->m_skyboxView;
	samplerInfo.sampler = this->m_sampler;

	DescriptorBufferInfo bufferInfo = { };
	bufferInfo.buffer = this->m_sunResultBuff;
	bufferInfo.nOffset = 0;
	bufferInfo.nRange = 0;

	this->m_descriptorSet->WriteTexture(0, 0, samplerInfo);
	this->m_descriptorSet->WriteBuffer(1, 0, bufferInfo);

	this->m_descriptorSet->UpdateWrites();
}

/**
* Creates sun extraction compute pipeline
*/
void 
SunExtraction::CreatePipeline() {
	Ref<Shader> shader = Shader::CreateShared();
	shader->LoadFromGLSL("SunExtraction.comp", EShaderStage::COMPUTE);

	ComputePipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shader = shader;
	pipelineInfo.descriptorSetLayouts = Vector{ this->m_setLayout };

	this->m_pipeline = this->m_device->CreateComputePipeline(pipelineInfo);
	this->m_pipelineLayout = this->m_pipeline->GetLayout();
}
