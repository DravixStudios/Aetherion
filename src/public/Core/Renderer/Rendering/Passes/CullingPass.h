#pragma once
#include "Core/Renderer/Rendering/Passes/BasePass.h"

class CullingPass : public BasePass {
public:
	void Init(Ref<Device> device) override;
	void Init(Ref<Device> device, uint32_t nFramesInFlight);
	void SetupNode(RenderGraphBuilder& builder) override;
	void Execute(
		Ref<GraphicsContext> context,
		RenderGraphContext& graphCtx,
		uint32_t nFrameIndex = 0
	) override;

	Ref<GPURingBuffer> GetInstanceBuffer() { return this->m_instanceBuffer; }
	Ref<GPURingBuffer> GetIndirectBuffer() { return this->m_indirectBuffer; }
	Ref<GPURingBuffer> GetBatchBuffer() { return this->m_batchBuffer; }
	Ref<GPURingBuffer> GetWVPBuffer() { return this->m_wvpBuffer; }
	Ref<GPUBuffer> GetCountBuffer() { return this->m_countBuffer; }

	void SetTotalBatches(uint32_t nBatchCount) { this->m_nTotalBatches = nBatchCount; }
	uint32_t GetTotalBatches() const { return this->m_nTotalBatches; }

	void SetViewProj(const glm::mat4& viewProj) { this->m_viewProj = viewProj; }
private:
	Ref<Device> m_device;

	/* Ring buffers */
	Ref<GPURingBuffer> m_instanceBuffer;
	Ref<GPURingBuffer> m_indirectBuffer;
	Ref<GPURingBuffer> m_batchBuffer;
	Ref<GPURingBuffer> m_wvpBuffer;

	Ref<GPURingBuffer> m_frustumBuffer;

	/* Count buffer */
	Ref<GPUBuffer> m_countBuffer;

	Vector<Ref<DescriptorSet>> m_cullingSets;
	Ref<Pipeline> m_computePipeline;

	uint32_t m_nTotalBatches = 0;

	Ref<DescriptorSetLayout> m_setLayout;

	uint32_t m_nFramesInFlight = 0;

	Vector<FrameIndirectData> m_frameData;

	Ref<DescriptorPool> m_pool;

	glm::mat4 m_viewProj = glm::mat4(1.f);

	void CreatePipeline();
	void CreateResources();
	void CreateDescriptors();

	void ExtractFrustumPlanes(const glm::mat4& viewProj, glm::vec4 planes[6]);
};