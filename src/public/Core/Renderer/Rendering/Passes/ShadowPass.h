#pragma once
#include "Core/Containers.h"
#include "Core/Renderer/Rendering/Passes/BasePass.h"
#include "Core/Renderer/Rendering/Passes/CullingPass.h"

#include <array>

static constexpr uint32_t CSM_CASCADE_COUNT = 4;
static constexpr uint32_t CSM_SHADOW_MAP_SIZE = 2048;

class ShadowPass : public BasePass {
public:
	struct CascadeData {
		glm::mat4 viewProj;
		float splitDepth;
	};

	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);

	void Resize(uint32_t nWidth, uint32_t nHeight);
	void SetupNode(RenderGraphBuilder& builder) override;

	void Execute(Ref<GraphicsContext> context, RenderGraphContext& graphCtx, uint32_t nFrameIdx = 0) override;

	void SetSunDirection(const glm::vec3& dir) { this->m_sunDirection = dir; }

	void SetCameraData(
		const glm::mat4& view,
		const glm::mat4& proj,
		float nearPlane,
		float farPlane
	);

	/**
	* Sets the reference to the main CullingPass
	* for reusing its batch/instance/WVP buffers
	* and the culling compute pipeline
	* 
	* @param pCullingPass Pointer to culling pass
	*/
	void SetCullingPass(CullingPass* pCullingPass) { this->m_pCullingPass = pCullingPass; }

	/**
	* Set scene data for draw pass (vertex/index buffers)
	*/
	void SetSceneData(
		Ref<DescriptorSet> sceneSet,
		Ref<DescriptorSetLayout> sceneSetLayout,
		Ref<GPUBuffer> vertexBuffer,
		Ref<GPUBuffer> indexBuffer
	);

	Ref<GPUTexture> GetShadowTexture() const { return this->m_shadowArray; }
	Ref<ImageView> GetShadowArrayView() const { return this->m_shadowArrayView; }
	Ref<Sampler> GetShadowSampler() const { return this->m_shadowSampler; }

	Ref<GPURingBuffer> GetCascadeBuffer() const { return this->m_cascadeBuff; }

	const std::array<CascadeData, CSM_CASCADE_COUNT>& GetCascades() const { return this->m_cascades; }

private:
	void CreateShadowResources();
	void CreateCullingResources();
	void CreatePipeline();
	void CalculateCascadeSplits();
	void CalculateCascadeViewProj(uint32_t nCascadeIdx, float nearSplit, float farSplit);
	void DispatchShadowCulling(Ref<GraphicsContext> context, uint32_t nCascadeIdx, uint32_t nFrameIdx);

	void ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 frustumPlanes[6]);

	glm::vec3 m_sunDirection = glm::vec3(0.f, -1.f, 0.f);

	glm::mat4 m_cameraView = glm::mat4(1.f);
	glm::mat4 m_cameraProj = glm::mat4(1.f);

	float m_nearPlane = .1f;
	float m_farPlane = 100.f;

	CullingPass* m_pCullingPass = nullptr;

	/* Cascade data */
	std::array<CascadeData, CSM_CASCADE_COUNT> m_cascades;

	/* Shadow map array */
	Ref<GPUTexture> m_shadowArray;
	Ref<ImageView> m_shadowArrayView;
	Vector<Ref<ImageView>> m_cascadeViews;
	Vector<Ref<Framebuffer>> m_cascadeFramebuffers;
	Ref<Sampler> m_shadowSampler;
	Ref<RenderPass> m_shadowRenderPass;

	/* Per-cascade GPU Culling */
	std::array<Ref<GPURingBuffer>, CSM_CASCADE_COUNT> m_shadowIndirectBuffers;
	std::array<Ref<GPUBuffer>, CSM_CASCADE_COUNT> m_shadowCountBuffers;

	/* Frustum ring buffer for the cascades */
	Ref<GPURingBuffer> m_shadowFrustumBuffer;

	/* Culling descriptors (reusing same layout as CullingPass) */
	Vector<Vector<Ref<DescriptorSet>>> m_shadowCullingSets;
	Ref<DescriptorPool> m_shadowCullingPool;

	/* Draw data */
	Ref<DescriptorSet> m_sceneSet;
	Ref<DescriptorSetLayout> m_sceneSetLayout;
	Ref<GPUBuffer> m_VBO;
	Ref<GPUBuffer> m_IBO;

	/* Cascade buffer */
	Ref<GPURingBuffer> m_cascadeBuff;

	uint32_t m_nFramesInFlight = 0;
	bool m_bResourcesCreated = false;
};