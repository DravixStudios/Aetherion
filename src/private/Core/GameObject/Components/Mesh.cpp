#include "Core/GameObject/Components/Mesh.h"
#include "Core/Core.h"
#include "Core/Renderer/Vulkan/VulkanRenderer.h"

#include <stb/stb_image.h>

Mesh::Mesh(String name) : Component::Component(name) { }

void 
Mesh::Start() {
	Component::Start();
}

void 
Mesh::Update() {
	Component::Update();
}

/*
	Load model from file (mainly FBX or GLB)
		NOTES:
			- If Mesh component already imported any Model, will throw error.
*/
bool 
Mesh::LoadModel(String filePath) {
	if (this->m_meshData.bLoaded) {
		Logger::Error("Mesh::LoadModel: Mesh already loaded");
		return false;
	}

	String exePath = GetExecutableDir();
	std::filesystem::path fullFilePath = std::filesystem::path(exePath) / filePath;

	/* Import our scene */
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fullFilePath.string().c_str(),
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenNormals |
		aiProcess_FlipUVs
	);

	/* If scene is null, get out and return false */
	if (scene == nullptr) {
		Logger::Error("Mesh::LoadModel: Scene couldn't be imported. Filename: {}", filePath.c_str());
		return false;
	}

	this->m_meshData.name = filePath;

	uint32_t nNumMeshes = scene->mNumMeshes; // Enumerate our mesh count

	/* Get each mesh from our scene */
	for (uint32_t i = 0; i < nNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		SubMeshData subData = { };

		subData.vertices.resize(mesh->mNumVertices);

		/* Vertices */
		for (uint32_t v = 0; v < mesh->mNumVertices; v++) {
			aiVector3D pos = mesh->mVertices[v];
			aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][v] : aiVector3D(0, 0, 0);
			aiVector3D normals = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0, 0, 0);

			subData.vertices[v] = {
				{ pos.x, pos.y, pos.z },
				{ normals.x, normals.y, normals.z },
				{ uv.x, uv.y }
			};
		}

		/* Indices */
		for (uint32_t f = 0; f < mesh->mNumFaces; f++) {
			aiFace& face = mesh->mFaces[f];
			for (uint32_t j = 0; j < face.mNumIndices; j++) {
				subData.indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
			}
		}

		/* Embedded textures */
		const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::function<void(aiTextureType, TextureData&)> loadEmbedded = [&](aiTextureType type, TextureData& out) {
			aiString path;
			if (material->GetTextureCount(type) > 0 && material->GetTexture(type, 0, &path) == AI_SUCCESS) {
				const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str());

				if (texture) {
					out.name = path.C_Str();
					out.nWidth = texture->mWidth;
					out.nHeight = texture->mHeight;
					out.bCompressed = (texture->mHeight == 0);

					size_t size = out.bCompressed ? texture->mWidth : (texture->mWidth * texture->mHeight * 4);
					out.data.resize(size);
					memcpy(out.data.data(), texture->pcData, size);
				}
			}
		};

		loadEmbedded(aiTextureType_DIFFUSE, subData.albedo);
		loadEmbedded(aiTextureType_METALNESS, subData.orm);
		loadEmbedded(aiTextureType_EMISSIVE, subData.emissive);

		this->m_meshData.subMeshes[i] = std::move(subData);
	}

	this->m_meshData.bLoaded = true;

	return true;
}