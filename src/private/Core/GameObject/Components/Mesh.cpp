#include "Core/GameObject/Components/Mesh.h"
#include "Core/Core.h"

#include <stb/stb_image.h>

Mesh::Mesh(std::string name) : Component::Component(name) {
	this->m_core = Core::GetInstance();
	this->m_bMeshImported = false;
	this->m_resourceManager = ResourceManager::GetInstance();
	this->m_bHasIndices = false;
}

void Mesh::Start() {
	Component::Start();
}

void Mesh::Update() {
	Component::Update();
}

/* 
	Load model from file (mainly FBX or GLB)
		NOTES:
			- If Mesh component already imported any Model, will throw error.
*/
bool Mesh::LoadModel(std::string filePath) {
	if (this->m_bMeshImported) {
		spdlog::error("[{0}] Mesh::LoadModel: Mesh already imported on this mesh component", this->m_name.c_str());
		throw std::runtime_error("Mesh::LoadModel: Mesh already imported on mesh");
		return false;
	}

	std::string exePath = GetExecutableDir();
	std::filesystem::path fullFilePath = std::filesystem::path(exePath) / filePath;

	/* Import our scene */
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fullFilePath.string().c_str(), NULL);
	
	/* If scene is null, get out and return false */
	if (scene == nullptr) {
		spdlog::error("Mesh::LoadModel: Scene couldn't be imported. Filename: {0}", filePath.c_str());
		return false;
	}

	Renderer* renderer = this->m_core->GetRenderer();
	
	uint32_t nNumMeshes = scene->mNumMeshes; // Enumerate our mesh count

	/* Get each mesh from our scene */
	for (uint32_t i = 0; i < nNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];

		/* Get the number of vertices */
		uint32_t nNumVertices = mesh->mNumVertices;

		std::vector<Vertex> vertices(nNumVertices);
		std::vector<uint16_t> indices;
		
		/*
			Store a new vertices on our vertices vector. 
				Formatted as specified Vertex struct at Util.h
		*/
		for (uint32_t x = 0; x < nNumVertices; x++) {
			aiVector3D aiVertex = mesh->mVertices[x];
			aiVector3D texCoords = { 0, 0, 0 };
			aiVector3D normals = { 0, 0, 0 };

			if (mesh->HasTextureCoords(0)) {
				texCoords = mesh->mTextureCoords[0][x];
			}

			if (mesh->HasNormals()) {
				normals = mesh->mNormals[x];
			}

			Vertex vertex = { { aiVertex.x, aiVertex.y, aiVertex.z }, { normals.x, normals.y, normals.z }, { texCoords.x, texCoords.y } };
			vertices[x] = vertex;
		
		}

		if (mesh->HasFaces()) {
			indices.reserve(mesh->mNumFaces * 3);
			this->m_bHasIndices = true;
			for (uint32_t x = 0; x < mesh->mNumFaces; x++) {
				aiFace& face = mesh->mFaces[x];
				for (uint32_t j = 0; j < face.mNumIndices; j++) {
					indices.push_back(face.mIndices[j]);
				}
			}

			GPUBuffer* IBO = renderer->CreateIndexBuffer(indices);
			this->m_IBOs[i] = IBO;
		}

		this->m_vertices[i] = vertices;
		GPUBuffer* VBO = renderer->CreateVertexBuffer(vertices);
		this->m_VBOs[i] = VBO;

		/* Texture loading */
		const aiMaterial* material =  scene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;

		/* Load albedo */
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 && material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {

			if (this->m_resourceManager->TextureExists(texturePath.C_Str())) {
				this->m_textures[i] = this->m_resourceManager->GetTexture(texturePath.C_Str());
				continue;
			}

			const aiTexture* texture = scene->GetEmbeddedTexture(texturePath.C_Str());

			GPUTexture* gpuTexture = this->m_resourceManager->UploadTexture(
				texturePath.C_Str(), 
				reinterpret_cast<const unsigned char*>(texture->pcData), 
				texture->mWidth, 
				texture->mHeight
			);

			if (VulkanRenderer* vkRenderer = dynamic_cast<VulkanRenderer*>(renderer)) {
				uint32_t nTextureIndex = vkRenderer->RegisterTexture(texturePath.C_Str(), gpuTexture);

				if (nTextureIndex == UINT32_MAX) {
					spdlog::error("Mesh::LoadModel: Vulkan renderer couldn't register texture {0}", texturePath.C_Str());
				}
				this->m_textureIndices[i] = nTextureIndex;
			}

			this->m_textures[i] = gpuTexture;
		}

		/* Load ORM */
		aiString metalPath;
		if (material->GetTextureCount(aiTextureType_METALNESS) > 0 && material->GetTexture(aiTextureType_METALNESS, 0, &metalPath) == AI_SUCCESS) {
			if (this->m_resourceManager->TextureExists(metalPath.C_Str())) {
				this->m_textures[i] = this->m_resourceManager->GetTexture(metalPath.C_Str());
				continue;
			}

			const aiTexture* texture = scene->GetEmbeddedTexture(metalPath.C_Str());

			GPUTexture* gpuTexture = this->m_resourceManager->UploadTexture(
				metalPath.C_Str(),
				reinterpret_cast<const unsigned char*>(texture->pcData),
				texture->mWidth,
				texture->mHeight
			);

			if (VulkanRenderer* vkRenderer = dynamic_cast<VulkanRenderer*>(renderer)) {
				uint32_t nTextureIndex = vkRenderer->RegisterTexture(metalPath.C_Str(), gpuTexture);

				if (nTextureIndex == UINT32_MAX) {
					spdlog::error("Mesh::LoadModel: Vulkan renderer couldn't register ORM texture {0}", metalPath.C_Str());
				}
				this->m_ormIndices[i] = nTextureIndex;
			}

			this->m_ormTextures[i] = gpuTexture;
		}

		/* 
			Load emissive
			TODO: Load emissive if it has, otherways, mark as no emissive.
		*/
		aiString emissivePath;
		if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0 && material->GetTexture(aiTextureType_EMISSIVE, 0, &emissivePath) == AI_SUCCESS) {
			if (this->m_resourceManager->TextureExists(emissivePath.C_Str())) {
				this->m_textures[i] = this->m_resourceManager->GetTexture(emissivePath.C_Str());
				continue;
			}

			const aiTexture* texture = scene->GetEmbeddedTexture(emissivePath.C_Str());

			GPUTexture* gpuTexture = this->m_resourceManager->UploadTexture(
				emissivePath.C_Str(),
				reinterpret_cast<const unsigned char*>(texture->pcData),
				texture->mWidth,
				texture->mHeight
			);

			if (VulkanRenderer* vkRenderer = dynamic_cast<VulkanRenderer*>(renderer)) {
				uint32_t nEmissiveIndex = this->RegisterVulkan(vkRenderer, emissivePath.C_Str(), gpuTexture);
				this->m_emissiveIndices[i] = nEmissiveIndex;
			}

			this->m_emissiveTextures[i] = gpuTexture;
		}
	}

	this->m_bMeshImported = true;
}

bool Mesh::HasIndices() {
	return this->m_bHasIndices;
}

/* Returns Mesh::m_VBOs */
std::map<uint32_t, GPUBuffer*>& Mesh::GetVBOs() {
	return this->m_VBOs;
}

/* Returns Mesh::m_IBOs */
std::map<uint32_t, GPUBuffer*>& Mesh::GetIBOs() {
	return this->m_IBOs;
}

/* Returns Mesh::m_textures */
std::map<uint32_t, GPUTexture*>& Mesh::GetTextures() {
	return this->m_textures;
}

/* Returns Mesh::m_textureIndices */
std::map<uint32_t, uint32_t>& Mesh::GetTextureIndices() {
	return this->m_textureIndices;
}

/* Returns Mesh::m_ormIndices */
std::map<uint32_t, uint32_t>& Mesh::GetORMIndices() {
	return this->m_ormIndices;
}

/* Returns Mesh::m_emissiveIndices */
std::map<uint32_t, uint32_t>& Mesh::GetEmissiveIndices() {
	return this->m_emissiveIndices;
}

/* Register a texture on vulkan renderer */
uint32_t Mesh::RegisterVulkan(VulkanRenderer* pRenderer, std::string textureName, GPUTexture* gpuTexture) {
	uint32_t nTextureIndex = pRenderer->RegisterTexture(textureName, gpuTexture);

	if (nTextureIndex == UINT32_MAX) {
		spdlog::error("Mesh::LoadModel: Vulkan renderer couldn't register texture {0}", textureName);
	}
	return nTextureIndex;
}