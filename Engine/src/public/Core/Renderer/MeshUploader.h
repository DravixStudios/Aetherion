#pragma once
#include "Core/Renderer/Device.h"
#include "Core/Renderer/MeshData.h"
#include "Core/Renderer/MegaBuffer.h"
#include "Core/Renderer/ResourceManager.h"

#include "Core/Containers.h"

#define INVALID_TEXTURE 0xFFFFFFFF

struct UploadedSubMeshMaterial {
	uint32_t nAlbedoIndex = INVALID_TEXTURE;
	uint32_t nORMIndex = INVALID_TEXTURE;
	uint32_t nEmissiveIndex = INVALID_TEXTURE;
	uint32_t nNormalIndex = INVALID_TEXTURE;

	glm::vec4 albedoColor;
	glm::vec4 emissiveColor;
	float ao;
	float roughness;
	float metallic;

	uint32_t materialFlags = 0;
};

struct UploadedSubMesh {
	MegaBufferAllocation geometry;
	UploadedSubMeshMaterial material;
	uint32_t nBlockIdx = 0;
};

struct UploadedMesh {
	Map<uint32_t, UploadedSubMesh> subMeshes;
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