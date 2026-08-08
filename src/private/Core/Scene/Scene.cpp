#include "Core/Scene/Scene.h"
#include "Core/Resources/SceneAsset.h"
#include "Core/Resources/GameObjectAsset.h"

Scene::Scene(const String& name) : m_name(name) {
	this->m_currentCamera = new EditorCamera("EditorCamera");

	/* Setup scene hierarchy */
	this->m_hierarchy = Hierarchy::Create(name);
}

void 
Scene::AddObject(GameObject* object) {
	String objName = object->GetName();

	if (this->m_gameObjects.count(objName) > 0) {
		spdlog::error("Scene::AddGameObject: GameObject with name {0} already exists", objName);
		return;
	}

	this->m_gameObjects[objName] = object;
	this->m_hierarchy.CreateNode(objName, this->m_hierarchy.root, object);
}

Map<String, GameObject*> 
Scene::GetObjects() {
	return this->m_gameObjects;
}

void 
Scene::DeleteObject(GameObject* pObj) {
	if (pObj == nullptr) return;

	String objName = pObj->GetName();

	if (this->m_gameObjects.count(objName) > 0) {
		this->m_gameObjects.erase(objName);
	}

	delete pObj;
}

void
Scene::Start() {
	Ref<Hierarchy::HierarchyNode> rootNode = this->m_hierarchy.root;
	this->StartHierarchy(rootNode);

	this->m_currentCamera->Start();
}

void 
Scene::StartHierarchy(Ref<Hierarchy::HierarchyNode> node) {
	if (node->pObj != nullptr) {
		node->pObj->Start();
	}

	if (!node->HasChildren())
		return;

	for (Ref<Hierarchy::HierarchyNode> children : node->children) {
		this->StartHierarchy(children);
	}
}

void
Scene::Update() {
	Ref<Hierarchy::HierarchyNode> rootNode = this->m_hierarchy.root;
	this->UpdateHierarchy(rootNode);

	this->m_currentCamera->Update();
}

void 
Scene::UpdateHierarchy(Ref<Hierarchy::HierarchyNode> node) {
	if (node->pObj != nullptr) {
		node->pObj->Update();
	}

	if (!node->HasChildren())
		return;

	for (Ref<Hierarchy::HierarchyNode> children : node->children) {
		this->UpdateHierarchy(children);
	}
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

/**
* Setup the scene from a serialized scene asset
* 
* @param sceneAsset Scene asset
*/
void 
Scene::SetupFromAsset(const SceneAsset& sceneAsset) {
	uint32_t nObjectCount = sceneAsset.header.nObjectCount;
	
	for (uint32_t i = 0; i < nObjectCount; i++) {
		const GameObjectAsset& objAsset = sceneAsset.objects[i];
		
		GameObject* pObj = new GameObject(String(objAsset.header.displayName));
		pObj->SetupFromAsset(objAsset);
		this->AddObject(pObj);
	}
}