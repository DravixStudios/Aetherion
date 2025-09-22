#include "Core/Scene/Scene.h"

Scene::Scene(std::string name) {
	this->m_name = name;
}

void Scene::AddObject(GameObject* object) {
	std::string objName = object->GetName();

	if (this->m_gameObjects.count(this->m_name) > 0) {
		spdlog::error("Scene::AddGameObject: GameObject with name {0} already exists", objName);
		return;
	}

	this->m_gameObjects[objName] = object;
}

void Scene::Start() {
	for (std::pair<std::string, GameObject*> obj : this->m_gameObjects) {
		obj.second->Start();
	}
}

void Scene::Update() {
	for (std::pair<std::string, GameObject*> obj : this->m_gameObjects) {
		obj.second->Update();
	}
}