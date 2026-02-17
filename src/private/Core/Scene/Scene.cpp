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