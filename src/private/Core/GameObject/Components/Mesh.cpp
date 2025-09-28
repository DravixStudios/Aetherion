#include "Core/GameObject/Components/Mesh.h"
#include "Core/Core.h"

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
	}

	this->m_bMeshImported = true;
}