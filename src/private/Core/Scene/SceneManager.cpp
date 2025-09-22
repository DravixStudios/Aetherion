#include "Core/Scene/SceneManager.h"

SceneManager* SceneManager::m_instance;

/* Constructor */
SceneManager::SceneManager() {
	this->m_currentScene = new Scene("SampleScene");
}

/* Add a scene to our scene map */
void SceneManager::AddScene(Scene* scene) {
	if (this->SceneExists(scene->m_name)) {
		spdlog::error("SceneManager::AddScene: Scene with name {0} already exists", scene->m_name);
		return;
	}

	this->m_scenes[scene->m_name] = scene;
}

/* Get scene by name */
Scene* SceneManager::GetScene(std::string name) {
	if (!this->SceneExists(name)) {
		spdlog::error("SceneManager::GetScene: Scene with name {0} not found", name);
		return nullptr;
	}
	
	return this->m_scenes[name];
}

/* Check if the scene actually exists */
bool SceneManager::SceneExists(std::string name) {
	return this->m_scenes.count(name) > 0;
}

Scene* SceneManager::GetCurrentScene() {
	return this->m_currentScene;
}

void SceneManager::Start() {
	this->m_currentScene->Start();
}

void SceneManager::Update() {
	this->m_currentScene->Update();
}

SceneManager* SceneManager::GetInstance() {
	if (SceneManager::m_instance == nullptr)
		SceneManager::m_instance = new SceneManager();
	return SceneManager::m_instance;
}