#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/GameObject/GameObject.h"
#include "Core/Camera/Camera.h"
#include "Core/Camera/EditorCamera.h"

class Scene {
	friend class SceneManager;
public:
	Scene(std::string name);

	void AddObject(GameObject* object);
	std::map<std::string, GameObject*> GetObjects();

	Camera* GetCurrentCamera();

	void Start();
	void Update();

private:
	std::string m_name;

	std::map<std::string, GameObject*> m_gameObjects;

	Camera* currentCamera;
	std::map<std::string, Camera*> m_cameras;
};