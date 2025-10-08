#include "Core/GameObject/Components/Mesh.h"
#include "Core/Core.h"

#include <stb_image.h>

Mesh::Mesh(std::string name) : Component::Component(name) {
	this->m_core = Core::GetInstance();
	this->m_bMeshImported = false;
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

	/* Import our scene */
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath, NULL);
	
	/* If scene is null, get out and return false */
	if (scene == nullptr) {
		spdlog::error("Mesh::LoadModel: Scene couldn't be imported. Filename: {0}", filePath.c_str());
		return false;
	}
	
	uint32_t nNumMeshes = scene->mNumMeshes; // Enumerate our mesh count

	/* Get each mesh from our scene */
	for (uint32_t i = 0; i < nNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		
		/* Get the number of vertices */
		uint32_t nNumVertices = mesh->mNumVertices;
		spdlog::debug("Mesh::LoadModel: Loading {0} vertices for mesh #{1} in file {2}", nNumVertices, i, filePath);

		std::vector<Vertex> vertices(nNumVertices);

		
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

		this->m_vertices[i] = vertices;

		/* Texture loading */
		const aiMaterial* material =  scene->mMaterials[mesh->mMaterialIndex];
		aiString texturePath;

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 && material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
			spdlog::debug("Mesh::LoadModel: Loading texture {0} for mesh #{1}", texturePath.C_Str(), i);

			const aiTexture* texture = scene->GetEmbeddedTexture(texturePath.C_Str());
			
			int nWidth = 0;
			int nHeight = 0;
			int nChannels = 0;

			unsigned char* pixelData = nullptr;

			/* If height is 0, means that is compressed (Width will be the full size of the image) */
			if (texture->mHeight == 0) {
				/* Load image from memory with STB_image */
				spdlog::debug("Mesh::LoadModel: Loading compressed texture {0} from memory", texturePath.C_Str());

				pixelData = stbi_load_from_memory(
					reinterpret_cast<const unsigned char*>(texture->pcData), // Buffer
					texture->mWidth,  // Size of the file
					&nWidth, // Pointer to width
					&nHeight, // Pointer to height
					&nChannels, // Pointer to channels
					4 // Desired channels
				);

				Renderer* renderer = this->m_core->GetRenderer();
				GPUBuffer* stagingBuffer = renderer->CreateStagingBuffer(pixelData, (nWidth * nHeight) * 4);
				GPUTexture* gpuTexture = renderer->CreateTexture(stagingBuffer, nWidth, nHeight, GPUFormat::RGBA8_SRGB);
			}
			else {
				/* TODO: Load uncompressed textures */
			}
		}
	}

	this->m_bMeshImported = true;
}