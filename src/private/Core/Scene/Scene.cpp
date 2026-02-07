#include "Core/Scene/Scene.h"

Scene::Scene(String name) {
	this->m_name = name;
	this->currentCamera = new EditorCamera("EditorCamera");
}

void 
Scene::AddObject(GameObject* object) {
	String objName = object->GetName();

	if (this->m_gameObjects.count(this->m_name) > 0) {
		spdlog::error("Scene::AddGameObject: GameObject with name {0} already exists", objName);
		return;
	}

	this->m_gameObjects[objName] = object;
}

Camera* 
Scene::GetCurrentCamera() {
	return this->currentCamera;
}

void 
Scene::CollectDrawData(
	Vector<ObjectInstanceData>& instances,
	Vector<DrawBatch>& batches,
	Vector<glm::mat4>& wvpMatrices,
	const glm::mat4 viewProj
) {
	for (auto& [name, gameObject] : this->m_gameObjects) {

		std::map<String, Component*> components = gameObject->GetComponents();
		std::map<String, Component*>::iterator it = components.find("Mesh");

		if (it == components.end()) continue;

		Mesh* mesh = dynamic_cast<Mesh*>(it->second);

		if (!mesh) continue;

		/* Calculate WVP */
		glm::mat4 world = gameObject->transform.GetWorldMatrix();
		glm::mat4 wvp = viewProj * world;
		
		for (auto& [idx, subMesh] : mesh->GetSubMeshes()) {
			/* WVP Index */
			uint32_t nWvpIdx = static_cast<uint32_t>(wvpMatrices.size());
			wvpMatrices.push_back(wvp);

			/* Instance data */
			ObjectInstanceData instance = { };
			instance.wvpOffset = nWvpIdx;
			instance.textureIndex = mesh->GetTextureIndices()[idx];
			instance.ormTextureIndex = mesh->GetORMIndices()[idx];
			instance.emissiveTextureIndex = mesh->GetEmissiveIndices()[idx];

			uint32_t nInstIdx = static_cast<uint32_t>(instances.size());
			instances.push_back(instance);

			DrawBatch batch = { };
			batch.indexCount = subMesh.nIndexCount;
			batch.firstIndex = subMesh.nFirstIndex;
			batch.vertexOffset = subMesh.nVertexOffset;
			batch.instanceDataIndex = nInstIdx;
			batches.push_back(batch);
		}
	}
}

std::map<String, GameObject*> 
Scene::GetObjects() {
	return this->m_gameObjects;
}

void
Scene::Start() {
	for (std::pair<String, GameObject*> obj : this->m_gameObjects) {
		obj.second->Start();
	}
	this->currentCamera->Start();
}

void
Scene::Update() {
	for (std::pair<String, GameObject*> obj : this->m_gameObjects) {
		obj.second->Update();
	}
	this->currentCamera->Update();
}