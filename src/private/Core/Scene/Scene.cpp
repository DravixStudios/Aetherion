#include "Core/Scene/Scene.h"
#include "Core/Resources/SceneAsset.h"
#include "Core/Resources/GameObjectAsset.h"

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

Map<String, GameObject*> 
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

/**
* Serialize scene data
* 
* @returns Serialized scene asset
*/
const SceneAsset 
Scene::SerializeScene() {
	/* Get GameObject count */
	uint32_t nObjectCount = this->m_gameObjects.size();
	std::vector<GameObjectAsset> assets(nObjectCount);

	SceneAsset sceneAsset = { };

	/* Serialize GameObjects */
	uint32_t i = 0;
	for (auto& [name, pObj] : this->m_gameObjects) {

		/* Create a GameObjectAsset */
		GameObjectAsset objAsset = { };
		objAsset.transform = pObj->transform;

		/* Serialize mesh asset */
		Map<String, Component*> components =  pObj->GetComponents();
		if (components.contains("MeshComponent")) {
			Mesh* meshComponent = dynamic_cast<Mesh*>(components["MeshComponent"]);

			if (meshComponent) {
				objAsset.header.displayName = pObj->GetName();
				objAsset.components = objAsset.components | EAssetComponent::MESH;
				objAsset.transform = pObj->transform;

				const AssetHandle meshHandle = meshComponent->GetAssetHandle();
				objAsset.meshHandle = meshHandle;
			}	
		}
		
		assets[i] = std::move(objAsset);
		i++;
	}

	if (i != nObjectCount) {
		Logger::Error("Scene::SerializeScene: Object count mismatch. Got {} of {}", i, nObjectCount);
		return SceneAsset{};
	}

	sceneAsset.header.displayName = this->GetName();
	sceneAsset.header.nObjectCount = nObjectCount;
	sceneAsset.objects = std::move(assets);

	return sceneAsset;
}