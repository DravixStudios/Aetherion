#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/GameObject/GameObject.h"
#include "Core/GameObject/Components/Mesh.h"
#include "Core/Camera/Camera.h"
#include "Core/Camera/EditorCamera.h"
#include "Utils.h"

class Scene {
	friend class SceneManager;
public:
	Scene(String name);

	void AddObject(GameObject* object);
	Map<String, GameObject*> GetObjects();

	Camera* GetCurrentCamera();

	void Start();
	void Update();

private:
	String m_name;

	Map<String, GameObject*> m_gameObjects;

	Camera* currentCamera;
	Map<String, Camera*> m_cameras;
};