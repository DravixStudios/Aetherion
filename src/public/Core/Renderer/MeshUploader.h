#pragma once
#include "Core/Renderer/Device.h"
#include "Core/Renderer/MeshData.h"
#include "Core/Renderer/MegaBuffer.h"
#include "Core/Renderer/ResourceManager.h"

struct UploadedSubMesh {
	MegaBufferAllocation geometry;
	uint32_t nAlbedoIndex = UINT32_MAX;
	uint32_t nORMIndex = UINT32_MAX;
	uint32_t nEmissiveIndex = UINT32_MAX;
	uint32_t nBlockIdx = 0;
};

struct UploadedMesh {
	std::map<uint32_t, UploadedSubMesh> subMeshes;
};

struct PendingTextureUpload {
	std::future<GPUTexture::Ptr> future;
	String hash;
	uint32_t nTextureIndex;
};

class MeshUploader {
public:
	void Init(
		Ref<Device> device,
		MegaBuffer* megaBuffer,
		Ref<DescriptorSet> bindlessSet,
		Ref<Sampler> defaultSampler
	);
	UploadedMesh Upload(const MeshData& meshData);

	void FinalizeUploads();
private:
	Ref<Device> m_device;
	MegaBuffer* m_megaBuffer;
	ResourceManager* m_resourceMgr;

	Ref<DescriptorSet> m_bindlessSet;
	Ref<Sampler> m_defaultSampler;
	
	uint32_t m_nNextTextureIndex = 0;

	Vector<Ref<GPUTexture>> m_textures;
	Vector<Ref<ImageView>> m_imageViews;

	Vector<PendingTextureUpload> m_pendingTextureUploads;

	uint32_t QueueTextureUpload(const TextureData& textureData);
};