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

	Ref<GPUTexture> GetAlbedo() const { return this->m_albedo; }
	Ref<GPUTexture> GetNormal() const { return this->m_normal; }
	Ref<GPUTexture> GetORM() const { return this->m_orm; }
	Ref<GPUTexture> GetEmissive() const { return this->m_emissive; }
	Ref<GPUTexture> GetBentNormal() const { return this->m_bentNormal; }
	Ref<GPUTexture> GetDepth() const { return this->m_depth; }

	Ref<ImageView> GetAlbedoView() const { return this->m_albedoView; }
	Ref<ImageView> GetNormalView() const { return this->m_normalView; }
	Ref<ImageView> GetORMView() const { return this->m_ormView; }
	Ref<ImageView> GetEmissiveView() const { return this->m_emissiveView; }
	Ref<ImageView> GetBentNormalView() const { return this->m_bentNormalView; }
	Ref<ImageView> GetDepthView() const { return this->m_depthView; }

	Ref<GPUTexture> GetAlbedoResolve() const { return this->m_albedoResolve; }
	Ref<GPUTexture> GetNormalResolve() const { return this->m_normalResolve; }
	Ref<GPUTexture> GetORMResolve() const { return this->m_ormResolve; }
	Ref<GPUTexture> GetEmissiveResolve() const { return this->m_emissiveResolve; }
	Ref<GPUTexture> GetBentNormalResolve() const { return this->m_bentNormalResolve; }
	Ref<GPUTexture> GetDepthResolve() const { return this->m_depthResolve; }

	Ref<ImageView> GetAlbedoResolveView() const { return this->m_albedoResolveView; }
	Ref<ImageView> GetNormalResolveView() const { return this->m_normalResolveView; }
	Ref<ImageView> GetORMResolveView() const { return this->m_ormResolveView; }
	Ref<ImageView> GetEmissiveResolveView() const { return this->m_emissiveResolveView; }
	Ref<ImageView> GetBentNormalResolveView() const { return this->m_bentNormalResolveView; }
	Ref<ImageView> GetDepthResolveView() const { return this->m_depthResolveView; }

	Ref<DescriptorSet> GetReadDescriptorSet() const { return this->m_readSet; }
	Ref<DescriptorSetLayout> GetReadLayout() const { return this->m_readLayout; }

	Ref<DescriptorSet> GetResolveDescriptorSet() const { return this->m_resolveReadSet; }
	Ref<DescriptorSetLayout> GetResolveLayout() const { return this->m_resolveReadLayout; }

	uint32_t GetWidth() const { return this->m_nWidth; }
	uint32_t GetHeight() const { return this->m_nHeight; }

	void SetSamples(ESampleCount samples) { m_samples = samples; }
	ESampleCount GetSamples() const { return this->m_samples; }
	bool IsMultiSampled() const { return this->m_isMultiSampled; }


private:
	Ref<GPUTexture> CreateTexture(GPUFormat format, ETextureUsage usage, ESampleCount samples = ESampleCount::SAMPLE_1);
	Ref<ImageView> CreateImageView(Ref<GPUTexture> texture, GPUFormat format, bool bIsDepth = false);
	void CreateTextures();
	void CreateDescriptors();

	void CreateResolveTargets();
	void CreateResolveDescriptors();

	Ref<Device> m_device;

	uint32_t m_nWidth = 0;
	uint32_t m_nHeight = 0;

	// G-Buffers
	Ref<GPUTexture> m_albedo;
	Ref<GPUTexture> m_normal;
	Ref<GPUTexture> m_orm;
	Ref<GPUTexture> m_emissive;
	Ref<GPUTexture> m_bentNormal;
	Ref<GPUTexture> m_depth;

	Ref<ImageView> m_albedoView;
	Ref<ImageView> m_normalView;
	Ref<ImageView> m_ormView;
	Ref<ImageView> m_emissiveView;
	Ref<ImageView> m_bentNormalView;
	Ref<ImageView> m_depthView;

	/*
	* Resolve targets
	*
	* NOTE: These resolve targets are going to be created only when
	* MSAA is enabled
	*/
	Ref<GPUTexture> m_albedoResolve;
	Ref<GPUTexture> m_normalResolve;
	Ref<GPUTexture> m_ormResolve;
	Ref<GPUTexture> m_emissiveResolve;
	Ref<GPUTexture> m_bentNormalResolve;
	Ref<GPUTexture> m_depthResolve;

	Ref<ImageView> m_albedoResolveView;
	Ref<ImageView> m_normalResolveView;
	Ref<ImageView> m_ormResolveView;
	Ref<ImageView> m_emissiveResolveView;
	Ref<ImageView> m_bentNormalResolveView;
	Ref<ImageView> m_depthResolveView;

	Ref<Sampler> m_sampler;
	Ref<DescriptorPool> m_pool;
	Ref<DescriptorSetLayout> m_readLayout;
	Ref<DescriptorSet> m_readSet;

	Ref<DescriptorPool> m_resolvePool;
	Ref<DescriptorSetLayout> m_resolveReadLayout;
	Ref<DescriptorSet> m_resolveReadSet;

	bool m_isMultiSampled = false;
	ESampleCount m_samples = ESampleCount::SAMPLE_1;
};