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
		throw std::runtime_error("Mesg::LoadModel: Mesh already imported on mesh");
		return false;
	}

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filePath, NULL);
	
	if (scene == nullptr) {
		spdlog::error("Mesh::LoadModel: Scene couldn't be imported. Filename: {0}", filePath.c_str());
		return false;
	}
	
	uint32_t nNumMeshes = scene->mNumMeshes;

	/* Get each mesh from our scene */
	for (uint32_t i = 0; i < nNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
	}
}