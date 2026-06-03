#include "Core/Scene/SceneManager.h"

SceneManager* SceneManager::m_instance;

/** 
* Constructor 
*/
SceneManager::SceneManager() {
	this->m_currentScene = new Scene("SampleScene");
}

/** 
* Add a scene to our scene map 
*/
void 
SceneManager::AddScene(Scene* scene) {
	if (this->SceneExists(scene->m_name)) {
		spdlog::error("SceneManager::AddScene: Scene with name {0} already exists", scene->m_name);
		return;
	}

	this->m_scenes[scene->m_name] = scene;
}

/** 
* Get scene by name
* 
* @param name Scene name
* 
* @returns Specified scene
*/
Scene* 
SceneManager::GetScene(String name) {
	if (!this->SceneExists(name)) {
		spdlog::error("SceneManager::GetScene: Scene with name {0} not found", name);
		return nullptr;
	}
	
	return this->m_scenes[name];
}

/** 
* Check if the specified scene exists 
* 
* @param name Scene name
* 
* @returns True if scene exists
*/
bool 
SceneManager::SceneExists(String name) {
	return this->m_scenes.count(name) > 0;
}

/**
* Get the current scene
* 
* @returns The current scene
*/
Scene* 
SceneManager::GetCurrentScene() {
	return this->m_currentScene;
}

/**
* Scene manager Start method
*/
void 
SceneManager::Start() {
	//GameObject* sampleObj = new GameObject("Sample object");

	//Mesh* mesh = new Mesh("MeshComponent");
	/*mesh->LoadModel("DamagedHelmet.glb");
	sampleObj->AddComponent("Mesh", mesh);*/
	//sampleObj->transform.scale = Vector3(.05f, .05f, .05f);
	//sampleObj->transform.Rotate(90.f, 0.f, 0.f);

	//this->m_currentScene->AddObject(sampleObj);
	this->m_currentScene->Start();
}

/**
* Scene manager Update method
*/
void 
SceneManager::Update() {
	this->m_currentScene->Update();
}

SceneManager* 
SceneManager::GetInstance() {
	if (SceneManager::m_instance == nullptr)
		SceneManager::m_instance = new SceneManager();
	return SceneManager::m_instance;
}

/**
* Sets the scene dimensions
* 
* Note: This only changes the 
* current camera dimensions
* 
* @param nWidth Width
* @param nHeight Height
*/
void 
SceneManager::SetDimensions(uint32_t nWidth, uint32_t nHeight) {
	this->m_currentScene->currentCamera->Resize(nWidth, nHeight);
}