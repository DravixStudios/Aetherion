#include "Core/Renderer/Rendering/Passes/ShadowPass.h"
#include "Core/Renderer/Rendering/RenderGraphContext.h"
#include "Core/Renderer/Rendering/RenderGraphBuilder.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <ranges>

/**
* Shadow pass initialization
* 
* @param device Logical device
*/
void
ShadowPass::Init(Ref<Device> device) {
	this->m_device = device;
}

/**
* Shadow pass initialization
* 
* @param device Logical device
* @param nFramesInFlight Frames in flight count
*/
void 
ShadowPass::Init(Ref<Device> device, uint32_t nFramesInFlight) {
	this->m_device = device;
	this->m_nFramesInFlight = nFramesInFlight;
}

/**
* Resizes the shadow pass
* 
* @param nWidth New width
* @param nHeight New height
*/
void
ShadowPass::Resize(uint32_t nWidth, uint32_t nHeight) {
	this->SetDimensions(nWidth, nHeight);
}

/**
* Shadow pass node setup
* 
* @param builder Render graph builder
*/
void 
ShadowPass::SetupNode(RenderGraphBuilder& builder) {
	builder.SetDimensions(this->m_nWidth, this->m_nHeight);
	builder.SetComputeOnly();
}

/**
* Executes the shadow pass
* 
* @param context Graphics context
* @param graphCtx Render graph context
* @param nFrameIdx Current frame index
*/
void 
ShadowPass::Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIdx) {
	this->m_shadowFrustumBuffer->Reset(nFrameIdx);

	this->CalculateCascadeSplits();

	struct CascadeShaderData {
		glm::mat4 cascadeViewProj[CSM_CASCADE_COUNT];
		glm::vec4 cascadeSplits; /* x=split0, y=split1, z=split2, w=split3 */
	} shaderData;

	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		shaderData.cascadeViewProj[i] = this->m_cascades[i].viewProj;
	}

	shaderData.cascadeSplits = glm::vec4(
		this->m_cascades[0].splitDepth,
		this->m_cascades[1].splitDepth,
		this->m_cascades[2].splitDepth,
		this->m_cascades[3].splitDepth
	);

	this->m_cascadeBuff->Reset(nFrameIdx);

	uint32_t nOffset = 0;
	void* pData = this->m_cascadeBuff->Allocate(sizeof(shaderData), nOffset);
	memcpy(pData, &shaderData, sizeof(shaderData));

	struct ShadowPushConstants {
		glm::mat4 lightViewProj;
		uint32_t nWvpAlignment;
	};

	Viewport vp = {
		0.f, 0.f,
		static_cast<float>(CSM_SHADOW_MAP_SIZE), static_cast<float>(CSM_SHADOW_MAP_SIZE),
		0.f, 1.f
	};

	Rect2D scissor = { { 0, 0 }, { CSM_SHADOW_MAP_SIZE , CSM_SHADOW_MAP_SIZE } };

	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		this->DispatchShadowCulling(context, i, nFrameIdx);

		ClearValue clearValue = { };
		clearValue.depthStencil.depth = 1.f;
		clearValue.depthStencil.stencil = 0;

		RenderPassBeginInfo beginInfo = { };
		beginInfo.renderPass = this->m_shadowRenderPass;
		beginInfo.framebuffer = this->m_cascadeFramebuffers[i];
		beginInfo.renderArea = scissor;
		beginInfo.clearValues = Vector{ clearValue };
		
		context->BeginRenderPass(beginInfo);
		context->BindPipeline(this->m_pipeline);
		context->SetViewport(vp);
		context->SetScissor(scissor);

		ShadowPushConstants pc = { };
		pc.lightViewProj = this->m_cascades[i].viewProj;
		pc.nWvpAlignment = this->m_pCullingPass->GetWVPBuffer()->GetAlignment();

		context->PushConstants(
			this->m_pipelineLayout,
			EShaderStage::VERTEX,
			0,
			sizeof(ShadowPushConstants),
			&pc
		);

		Ref<GPURingBuffer> indirectBuff = this->m_shadowIndirectBuffers[i];
		indirectBuff->Reset(nFrameIdx);
		context->BindDescriptorSets(0, { this->m_sceneSet });

		uint32_t nBaseOffset = indirectBuff->GetPerFrameSize()* nFrameIdx;
		uint32_t nCurrentOffset = nBaseOffset;
		uint32_t nTotalBatches = this->m_pCullingPass->GetTotalBatches();

		for (uint32_t j = 0; j < this->m_nBlockCount; j++) {
			Ref<GPUBuffer> VBO = this->m_blocks[j].vertexBuffer;
			Ref<GPUBuffer> IBO = this->m_blocks[j].indexBuffer;

			context->BindVertexBuffers({ VBO });
			context->BindIndexBuffer(IBO, EIndexType::UINT32);

			context->DrawIndexedIndirect(
				indirectBuff->GetBuffer(),
				nCurrentOffset,
				this->m_shadowCountBuffers[i],
				j * sizeof(uint32_t),
				this->m_pCullingPass->GetMaxBatchesPerBlock(),
				sizeof(DrawIndexedIndirectCommand)
			);

			nCurrentOffset += this->m_pCullingPass->GetMaxBatchesPerBlock() * sizeof(DrawIndexedIndirectCommand);
		}

		context->EndRenderPass();
	}

	context->GlobalBarrier();
}

void
ShadowPass::SetCameraData(
	const glm::mat4& view,
	const glm::mat4& proj,
	float nearPlane,
	float farPlane
) {
	this->m_cameraView = view;
	this->m_cameraProj = proj;
	this->m_nearPlane = nearPlane;
	this->m_farPlane = farPlane;
}

void
ShadowPass::SetSceneData(
	Ref<DescriptorSet> sceneSet,
	Ref<DescriptorSetLayout> sceneSetLayout,
	const Vector<MegaBuffer::Block>& blocks,
	uint32_t nBlockCount
) {
	this->m_sceneSet = sceneSet;
	this->m_sceneSetLayout = sceneSetLayout;
	this->m_blocks = blocks;
	this->m_nBlockCount = nBlockCount;

	if(!this->m_bResourcesCreated) {
		this->CreateShadowResources();
		this->CreateCullingResources();
		this->CreatePipeline();
	}
}

/**
* Calculate cascade splits (Logarithmic PSSM)
*/
void 
ShadowPass::CalculateCascadeSplits() {
	/*
		PSSM (Practical Split Scheme) combines
		logarithmic and uniform distribution for
		better results

		lambda = blend factor (0 = uniform, 1 = logarithmic)
	*/
	constexpr float lambda = 0.7f;

	float splits[CSM_CASCADE_COUNT + 1];
	splits[0] = this->m_nearPlane;

	for (uint32_t i = 1; i <= CSM_CASCADE_COUNT; i++) {
		float p = static_cast<float>(i) / static_cast<float>(CSM_CASCADE_COUNT);

		float logSplit = this->m_nearPlane * std::pow(this->m_farPlane / this->m_nearPlane, p);

		float uniformSplit = this->m_nearPlane + (this->m_farPlane - this->m_nearPlane) * p;

		splits[i] = lambda * logSplit + (1.f - lambda) * uniformSplit;
	}

	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		this->m_cascades[i].splitDepth = splits[i + 1];
		this->CalculateCascadeViewProj(i, splits[i], splits[i + 1]);
	}
}

/**
* Calculates the stable view-projection matrix for a shadow cascade.
*
* Steps:
* - Construct the cascade sub-frustum in world space.
* - Compute the bounding sphere (center and radius) enclosing the sub-frustum.
* - Construct a light view matrix looking at the sphere center from the sun direction.
* - Create a rotation-invariant orthographic projection centered on the sphere.
* - Apply texel snapping to the projection matrix to eliminate shimmering during translation.
* 
* @param nCascadeIdx Cascade index
* @param nearSplit Near split
* @param farSplit Far split
*/
void
ShadowPass::CalculateCascadeViewProj(
	uint32_t nCascadeIdx,
	float nearSplit,
	float farSplit
) {
	/* Inverse camera viewProj */
	glm::mat4 invVP = glm::inverse(this->m_cameraProj * this->m_cameraView);

	/* 8 frustum corners in NDC */
	glm::vec4 frustumCorners[8] = {
		/* Near plane */
		{ -1.f, -1.f, 0.f, 1.f },
		{ 1.f, -1.f, 0.f, 1.f },
		{ 1.f, 1.f, 0.f, 1.f },
		{ -1.f, 1.f, 0.f, 1.f },

		/* Far plane */
		{ -1.f, -1.f, 1.f, 1.f },
		{ 1.f, -1.f, 1.f, 1.f },
		{ 1.f, 1.f, 1.f, 1.f },
		{ -1.f, 1.f, 1.f, 1.f }
	};

	/* Transform to world space */
	glm::vec3 worldCorners[8];
	for (uint32_t i = 0; i < 8; i++) {
		glm::vec4 w = invVP * frustumCorners[i];
		worldCorners[i] = glm::vec3(w) / w.w;
	}

	float nearRatio = (nearSplit - this->m_nearPlane) / (this->m_farPlane - this->m_nearPlane);
	float farRatio = (farSplit - this->m_nearPlane) / (this->m_farPlane - this->m_nearPlane);

	glm::vec3 cascadeCorners[8];
	for (uint32_t i = 0; i < 4; i++) {
		glm::vec3 ray = worldCorners[i + 4] - worldCorners[i];
		cascadeCorners[i] = worldCorners[i] + ray * nearRatio;
		cascadeCorners[i + 4] = worldCorners[i] + ray * farRatio;
	}

	/* Use the center of the bounding sphere for maximum stability */
	glm::vec3 center(0.f);
	for (const glm::vec3& corner : cascadeCorners) {
		center += corner;
	}
	center /= 8.f;

	/* Calculate radius of the bounding sphere */
	float frustumDiagonal = glm::distance(cascadeCorners[0], cascadeCorners[6]);
	float radius = frustumDiagonal * .5f;
	radius = std::ceil(radius * 16.f) / 16.f; // Quantize radius

	/* Stable light direction and view matrix */
	glm::vec3 lightDir = glm::normalize(this->m_sunDirection);
	glm::vec3 up = (std::abs(lightDir.y) > .99f) ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);

	glm::mat4 lightView = glm::lookAt(
		center + lightDir * radius, 
		center,
		up
	);

	/* Orthographic projection centered on the sphere */
	glm::mat4 lightOrtho = glm::ortho(-radius, radius, -radius, radius, -radius * 100.f, radius * 100.f);

	/* Correction Matrix (Flip Y + Map Z 0..1) */
	glm::mat4 correction = glm::mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f
	);
	lightOrtho = correction * lightOrtho;

	/*  Eliminate shimmering by snapping the light-space origin to texel increments */
	glm::mat4 shadowMatrix = lightOrtho * lightView;
	glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f);
	
	float shadowMapSize = static_cast<float>(CSM_SHADOW_MAP_SIZE);
	shadowOrigin *= shadowMapSize / 2.f;

	glm::vec4 roundedOrigin = glm::round(shadowOrigin);
	glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
	roundOffset = roundOffset * (2.f / shadowMapSize);
	roundOffset.z = 0.f;
	roundOffset.w = 0.f;

	lightOrtho[3] += roundOffset;

	this->m_cascades[nCascadeIdx].viewProj = lightOrtho * lightView;
}

/**
* Creates shadow resources
*/
void
ShadowPass::CreateShadowResources() {
	/* Texture array for the cascades */
	TextureCreateInfo texInfo = { };
	texInfo.extent = { CSM_SHADOW_MAP_SIZE, CSM_SHADOW_MAP_SIZE, 1 };
	texInfo.format = GPUFormat::D32_FLOAT;
	texInfo.nMipLevels = 1;
	texInfo.nArrayLayers = CSM_CASCADE_COUNT;
	texInfo.samples = ESampleCount::SAMPLE_1;
	texInfo.tiling = ETextureTiling::OPTIMAL;
	texInfo.usage = ETextureUsage::DEPTH_STENCIL_ATTACHMENT | ETextureUsage::SAMPLED;
	texInfo.sharingMode = ESharingMode::EXCLUSIVE;
	texInfo.imageType = ETextureDimensions::TYPE_2D;
	texInfo.initialLayout = ETextureLayout::UNDEFINED;
	
	this->m_shadowArray = this->m_device->CreateTexture(texInfo);

	/* Image view for the whole array */
	ImageViewCreateInfo arrayViewInfo = { };
	arrayViewInfo.image = this->m_shadowArray;
	arrayViewInfo.viewType = EImageViewType::TYPE_2D_ARRAY;
	arrayViewInfo.format = GPUFormat::D32_FLOAT;
	arrayViewInfo.subresourceRange.aspectMask = EImageAspect::DEPTH;
	arrayViewInfo.subresourceRange.nBaseArrayLayer = 0;
	arrayViewInfo.subresourceRange.nLayerCount = CSM_CASCADE_COUNT;
	arrayViewInfo.subresourceRange.nBaseMipLevel = 0;
	arrayViewInfo.subresourceRange.nLevelCount = 1;

	this->m_shadowArrayView = this->m_device->CreateImageView(arrayViewInfo);

	/* One image view per cascade */
	this->m_cascadeViews.resize(CSM_CASCADE_COUNT);
	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		ImageViewCreateInfo viewInfo = { };
		viewInfo.format = GPUFormat::D32_FLOAT;
		viewInfo.image = this->m_shadowArray;
		viewInfo.viewType = EImageViewType::TYPE_2D;
		viewInfo.subresourceRange.aspectMask = EImageAspect::DEPTH;
		viewInfo.subresourceRange.nBaseArrayLayer = i;
		viewInfo.subresourceRange.nLayerCount = 1;
		viewInfo.subresourceRange.nBaseMipLevel = 0;
		viewInfo.subresourceRange.nLevelCount = 1;

		this->m_cascadeViews[i] = this->m_device->CreateImageView(viewInfo);
	}

	/* Create shadow sampler */
	SamplerCreateInfo samplerInfo = { };
	samplerInfo.minFilter = EFilter::LINEAR;
	samplerInfo.magFilter = EFilter::LINEAR;
	samplerInfo.mipmapMode = EMipmapMode::MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = EAddressMode::CLAMP_TO_BORDER;
	samplerInfo.addressModeV = EAddressMode::CLAMP_TO_BORDER;
	samplerInfo.addressModeW = EAddressMode::CLAMP_TO_BORDER;
	samplerInfo.borderColor = EBorderColor::FLOAT_OPAQUE_WHITE;
	samplerInfo.bCompareEnable = true;
	samplerInfo.compareOp = ECompareOp::LESS_OR_EQUAL;
	samplerInfo.bAnisotropyEnable = false;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 1.f;

	this->m_shadowSampler = this->m_device->CreateSampler(samplerInfo);

	/* Shadow render pass */
	AttachmentDescription depthAttachment = { };
	depthAttachment.format = GPUFormat::D32_FLOAT;
	depthAttachment.sampleCount = ESampleCount::SAMPLE_1;
	depthAttachment.initialLayout = EImageLayout::UNDEFINED;
	depthAttachment.finalLayout = EImageLayout::SHADER_READ_ONLY;
	depthAttachment.loadOp = EAttachmentLoadOp::CLEAR;
	depthAttachment.storeOp = EAttachmentStoreOp::STORE;
	depthAttachment.stencilLoadOp = EAttachmentLoadOp::DONT_CARE;
	depthAttachment.stencilStoreOp = EAttachmentStoreOp::DONT_CARE;
	
	SubpassDescription subpass = { };
	subpass.depthStencilAttachment = { 0, EImageLayout::DEPTH_STENCIL_ATTACHMENT };
	subpass.colorAttachments = { };
	subpass.bHasDepthStencil = true;

	RenderPassCreateInfo rpInfo = { };
	rpInfo.attachments = Vector{ depthAttachment };
	rpInfo.subpasses = Vector{ subpass };
	
	this->m_shadowRenderPass = this->m_device->CreateRenderPass(rpInfo);

	/* Create framebuffers */
	this->m_cascadeFramebuffers.resize(CSM_CASCADE_COUNT);
	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		FramebufferCreateInfo fbInfo = { };
		fbInfo.renderPass = this->m_shadowRenderPass;
		fbInfo.attachments = { this->m_cascadeViews[i] };
		fbInfo.nWidth = CSM_SHADOW_MAP_SIZE;
		fbInfo.nHeight = CSM_SHADOW_MAP_SIZE;
		fbInfo.nLayers = 1;
		
		this->m_cascadeFramebuffers[i] = this->m_device->CreateFramebuffer(fbInfo);
	}

	/*
		Cascade data UBO

		CascadeData {
			mat4 cascadeViewProj[4];
			vec4 cascadeSplits;
		}
	*/
	uint32_t nUBOSize = sizeof(glm::mat4) * CSM_CASCADE_COUNT + sizeof(glm::vec4);
	uint32_t nAlignedSize = NextPowerOf2(nUBOSize);

	RingBufferCreateInfo uboInfo = { };
	uboInfo.nAlignment = nAlignedSize;
	uboInfo.nBufferSize = nAlignedSize * this->m_nFramesInFlight;
	uboInfo.nFramesInFlight = this->m_nFramesInFlight;
	uboInfo.usage = EBufferUsage::UNIFORM_BUFFER;

	this->m_cascadeBuff = this->m_device->CreateRingBuffer(uboInfo);

	this->m_bResourcesCreated = true;
}

void
ShadowPass::CreateCullingResources() {
	constexpr uint32_t MAX_DRAWS = 131072;

	/* One indirect buffer and count buffer per cascade */
	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		/* Create indirect buffer */
		RingBufferCreateInfo indirectInfo = { };
		indirectInfo.usage = EBufferUsage::TRANSFER_DST | EBufferUsage::INDIRECT_BUFFER | EBufferUsage::STORAGE_BUFFER;
		indirectInfo.nAlignment = 32;
		indirectInfo.nFramesInFlight = this->m_nFramesInFlight;
		indirectInfo.nBufferSize = MAX_DRAWS * sizeof(DrawIndexedIndirectCommand);
		
		this->m_shadowIndirectBuffers[i] = this->m_device->CreateRingBuffer(indirectInfo);

		/* Create count buffer */
		BufferCreateInfo countInfo = { };
		countInfo.nSize = sizeof(uint32_t) * 64;
		countInfo.sharingMode = ESharingMode::EXCLUSIVE;
		countInfo.usage = EBufferUsage::STORAGE_BUFFER | EBufferUsage::INDIRECT_BUFFER | EBufferUsage::TRANSFER_DST;
		countInfo.type = EBufferType::STORAGE_BUFFER;

		this->m_shadowCountBuffers[i] = this->m_device->CreateBuffer(countInfo);
	}

	/* Frustum ring buffer */
	uint32_t nFrustumSize = sizeof(FrustumData);
	uint32_t nAligned = NextPowerOf2(nFrustumSize);

	RingBufferCreateInfo frustumInfo = { };
	frustumInfo.nAlignment = nAligned;
	frustumInfo.nFramesInFlight = this->m_nFramesInFlight;
	frustumInfo.usage = EBufferUsage::STORAGE_BUFFER;
	frustumInfo.nBufferSize = nAligned * CSM_CASCADE_COUNT * this->m_nFramesInFlight;

	this->m_shadowFrustumBuffer = this->m_device->CreateRingBuffer(frustumInfo);

	/* Create descriptor pool */
	DescriptorPoolSize poolSize = { };
	poolSize.nDescriptorCount = 6 * CSM_CASCADE_COUNT * this->m_nFramesInFlight;
	poolSize.type = EDescriptorType::STORAGE_BUFFER;

	DescriptorPoolCreateInfo poolInfo = { };
	poolInfo.nMaxSets = CSM_CASCADE_COUNT * this->m_nFramesInFlight;
	poolInfo.poolSizes = Vector{ poolSize };
	
	this->m_shadowCullingPool = this->m_device->CreateDescriptorPool(poolInfo);

	/* Reuse same layout as CullingPass */
	Ref<DescriptorSetLayout> cullingLayout = this->m_pCullingPass->GetSetLayout();

	this->m_shadowCullingSets.resize(CSM_CASCADE_COUNT);

	std::ranges::for_each(this->m_shadowCullingSets, [this](Vector<Ref<DescriptorSet>>& set) {
		set.resize(this->m_nFramesInFlight);
	});

	/*
		Allocate descriptor sets

		Bindings:
		 0. Instance data (Reused)
		 1. Batch data (Reused)
		 2. Output commands (One per cascade)
		 3. Draw count (One per cascade)
		 4. WVP Data (Reused)
		 5. Frustum data (From the Shader pass)
	*/

	for (uint32_t i = 0; i < CSM_CASCADE_COUNT; i++) {
		/* Define per frame sizes */
		std::array<uint32_t, 6> frameSizes = {
			this->m_pCullingPass->GetInstanceBuffer()->GetPerFrameSize(), // 0
			this->m_pCullingPass->GetBatchBuffer()->GetPerFrameSize(), // 1
			this->m_shadowIndirectBuffers[i]->GetPerFrameSize(), // 2
			0, // 3 (Is not per frame)
			this->m_pCullingPass->GetWVPBuffer()->GetPerFrameSize(), // 4
			this->m_shadowFrustumBuffer->GetPerFrameSize(), // 5
		};

		/* Create a descriptor set per frame in flight */
		Vector<Ref<DescriptorSet>>& sets = this->m_shadowCullingSets[i];
		for (uint32_t j = 0; j < this->m_nFramesInFlight; j++) {
			sets[j] = this->m_device->CreateDescriptorSet(this->m_shadowCullingPool, cullingLayout);

			Ref<DescriptorSet>& set = sets[j];
			/* Binding 0: Instance data */
			set->WriteBuffer(0, 0, {
				this->m_pCullingPass->GetInstanceBuffer()->GetBuffer(), frameSizes[0] * j, frameSizes[0]
				});

			/* Binding 1: Batch data */
			set->WriteBuffer(1, 0, {
				this->m_pCullingPass->GetBatchBuffer()->GetBuffer(), frameSizes[1] * j, frameSizes[1]
				});

			/* Binding 2: Output commands */
			set->WriteBuffer(2, 0, {
				this->m_shadowIndirectBuffers[i]->GetBuffer(), frameSizes[2] * j, frameSizes[2]
				});

			/* Binding 3: Draw count */
			set->WriteBuffer(3, 0, {
				this->m_shadowCountBuffers[i], frameSizes[3] * j, 64 * sizeof(uint32_t)
			});

			/* Binding 4: WVP Data */
			set->WriteBuffer(4, 0, {
				this->m_pCullingPass->GetWVPBuffer()->GetBuffer(), frameSizes[4] * j, frameSizes[4]
			});

			/* Binding 5: Frustum data */
			set->WriteBuffer(5, 0, {
				this->m_shadowFrustumBuffer->GetBuffer(), frameSizes[5] * j, frameSizes[5]
			});

			set->UpdateWrites();
		}

	}
}

/**
* Dispatches shadow culling
* 
* @param context Graphics context
* @param nCascadeIdx Cascade index
* @param nFrameIdx Frame index
*/
void 
ShadowPass::DispatchShadowCulling(Ref<GraphicsContext> context, uint32_t nCascadeIdx, uint32_t nFrameIdx) {
	/* Reset count buffer to 0 */
	Ref<GPUBuffer> countBuffer = this->m_shadowCountBuffers[nCascadeIdx];
	context->FillBuffer(countBuffer, 0, sizeof(uint32_t) * 64, 0);
	context->BufferMemoryBarrier(countBuffer, EAccess::TRANSFER_WRITE, EAccess::SHADER_WRITE);

	CascadeData cascade = this->m_cascades[nCascadeIdx];

	/* Calculate frustum planes for this cascade */
	glm::vec4 frustumPlanes[6];
	this->ExtractFrustumPlanes(cascade.viewProj, frustumPlanes);

	FrustumData frustumData = { };
	frustumData.viewProj = cascade.viewProj;
	memcpy(frustumData.frustumPlanes, frustumPlanes, sizeof(frustumPlanes));

	/* Upload frustum data to the ring buffer */
	uint32_t nFrustumOffset = 0;
	void* pFrustumData = this->m_shadowFrustumBuffer->Allocate(sizeof(frustumData), nFrustumOffset);

	memcpy(pFrustumData, &frustumData, sizeof(frustumData));

	context->BindPipeline(this->m_pCullingPass->GetPipeline());
	context->BindDescriptorSets(0, { this->m_shadowCullingSets[nCascadeIdx][nFrameIdx]});

	/* Push constants */
	struct {
		uint32_t nTotalBatches;
		uint32_t nWvpAlignment;
		uint32_t nFrustumOffset;
		uint32_t nFrustumAlignment;
		uint32_t nMaxDrawsPerBlock;
	} pushData;


	pushData.nTotalBatches = this->m_pCullingPass->GetTotalBatches();
	pushData.nWvpAlignment = this->m_pCullingPass->GetWVPBuffer()->GetAlignment();
	pushData.nFrustumOffset = nFrustumOffset;
	pushData.nFrustumAlignment = this->m_shadowFrustumBuffer->GetAlignment();
	pushData.nMaxDrawsPerBlock = this->m_pCullingPass->GetMaxBatchesPerBlock();

	context->PushConstants(
		this->m_pCullingPass->GetPipelineLayout(),
		EShaderStage::COMPUTE,
		0,
		sizeof(pushData), &pushData
	);

	uint32_t nGroups = (pushData.nTotalBatches + 255) / 256;
	context->Dispatch(nGroups, 1, 1);

	context->BufferMemoryBarrier(
		this->m_shadowIndirectBuffers[nCascadeIdx]->GetBuffer(),
		EAccess::SHADER_WRITE, 
		EAccess::INDIRECT_COMMAND_READ
	);

	context->BufferMemoryBarrier(
		this->m_shadowCountBuffers[nCascadeIdx],
		EAccess::SHADER_WRITE,
		EAccess::INDIRECT_COMMAND_READ
	);
}

/**
* Creates shadow pass pipeline (depth only)
*/
void
ShadowPass::CreatePipeline() {
	/* Compile shaders */
	Ref<Shader> vertexShader = Shader::CreateShared();
	vertexShader->LoadFromGLSL("ShadowPass.vert", EShaderStage::VERTEX);

	/* Create pipeline layout */
	PushConstantRange pushRange = { };
	pushRange.nSize = sizeof(glm::mat4) + sizeof(uint32_t);
	pushRange.stage = EShaderStage::VERTEX;
	
	PipelineLayoutCreateInfo layoutInfo = { };
	layoutInfo.setLayouts = Vector{ this->m_sceneSetLayout };
	layoutInfo.pushConstantRanges = Vector{ pushRange };

	this->m_pipelineLayout = this->m_device->CreatePipelineLayout(layoutInfo);

	/* Create pipeline */
	Vector<VertexInputBinding> bindings(1);
	bindings[0].nBinding = 0;
	bindings[0].nStride = sizeof(Vertex);

	Vector<VertexInputAttribute> attribs(1);
	attribs[0].format = GPUFormat::RGB32_FLOAT;
	attribs[0].nLocation = 0;
	attribs[0].nBinding = 0;
	attribs[0].nOffset = offsetof(Vertex, position);

	GraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.shaders = Vector{ vertexShader };
	pipelineInfo.vertexBindings = bindings;
	pipelineInfo.vertexAttributes = attribs;

	pipelineInfo.rasterizationState.cullMode = ECullMode::FRONT;
	pipelineInfo.rasterizationState.frontFace = EFrontFace::CLOCKWISE;
	pipelineInfo.rasterizationState.bDepthBiasEnable = true;
	pipelineInfo.rasterizationState.depthBiasConstantFactor = 2.f;
	pipelineInfo.rasterizationState.depthBiasSlopeFactor = 1.2f;
	pipelineInfo.rasterizationState.polygonMode = EPolygonMode::FILL;

	pipelineInfo.depthStencilState.bDepthTestEnable = true;
	pipelineInfo.depthStencilState.bDepthWriteEnable = true;
	pipelineInfo.depthStencilState.depthCompareOp = ECompareOp::LESS_OR_EQUAL;

	/* No color attachments */
	pipelineInfo.colorBlendState.attachments = { };
	pipelineInfo.depthFormat = GPUFormat::D32_FLOAT;

	pipelineInfo.renderPass = this->m_shadowRenderPass;
	pipelineInfo.pipelineLayout = this->m_pipelineLayout;
	pipelineInfo.nSubpass = 0;

	this->m_pipeline = this->m_device->CreateGraphicsPipeline(pipelineInfo);
}

/**
* Extract frustum planes
* 
* @param viewProj Camera view projection
* @param planes Out frustum planes array (size = 6)
*/
void
ShadowPass::ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 planes[6]) {
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