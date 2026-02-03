#pragma once
#include "Core/Renderer/Device.h"
#include "Core/Renderer/GPUTexture.h"
#include "Core/Renderer/ImageView.h"
#include "Core/Renderer/DescriptorSet.h"
#include "Core/Renderer/DescriptorPool.h"
#include "Core/Renderer/DescriptorSetLayout.h"
#include "Core/Renderer/Sampler.h"
#include "Core/Renderer/Rendering/GBuffer/GBufferLayout.h"

class GBufferManager {
public:
	void Init(Ref<Device> device, uint32_t nWidth, uint32_t nHeight);
	void Resize(uint32_t nWidth, uint32_t nHeight);
	void Shutdown();

	Ref<GPUTexture> GetAlbedo() const { return this->m_albedo; }
	Ref<GPUTexture> GetNormal() const { return this->m_normal; }
	Ref<GPUTexture> GetORM() const { return this->m_orm; }
	Ref<GPUTexture> GetEmissive() const { return this->m_emissive; }
	Ref<GPUTexture> GetPosition() const { return this->m_position; }
	Ref<GPUTexture> GetDepth() const { return this->m_depth; }

	Ref<ImageView> GetAlbedoView() const { return this->m_albedoView; }
	Ref<ImageView> GetNormalView() const { return this->m_normalView; }
	Ref<ImageView> GetORMView() const { return this->m_ormView; }
	Ref<ImageView> GetEmissiveView() const { return this->m_emissiveView; }
	Ref<ImageView> GetPositionView() const { return this->m_positionView; }
	Ref<ImageView> GetDepthView() const { return this->m_depthView; }

	Ref<DescriptorSet> GetReadDescriptorSet() const { return this->m_readSet; }
	Ref<DescriptorSetLayout> GetReadLayout() const { return this->m_readLayout; }

	uint32_t GetWidth() const { return this->m_nWidth; }
	uint32_t GetHeight() const { return this->m_nHeight; }

private:
	void CreateTextures();
	void CreateDescriptors();

	Ref<Device> m_device;

	uint32_t m_nWidth = 0;
	uint32_t m_nHeight = 0;

	Ref<GPUTexture> m_albedo;
	Ref<GPUTexture> m_normal;
	Ref<GPUTexture> m_orm;
	Ref<GPUTexture> m_emissive;
	Ref<GPUTexture> m_position;
	Ref<GPUTexture> m_depth;

	Ref<ImageView> m_albedoView;
	Ref<ImageView> m_normalView;
	Ref<ImageView> m_ormView;
	Ref<ImageView> m_emissiveView;
	Ref<ImageView> m_positionView;
	Ref<ImageView> m_depthView;

	Ref<Sampler> m_sampler;
	Ref<DescriptorPool> m_pool;
	Ref<DescriptorSetLayout> m_readLayout;
	Ref<DescriptorSet> m_readSet;
};