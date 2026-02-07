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
	std::map<String, GameObject*> GetObjects();

	Camera* GetCurrentCamera();

	void CollectDrawData(
		Vector<ObjectInstanceData>& instances,
		Vector<DrawBatch>& batches,
		Vector<glm::mat4>& wvpMatrices,
		const glm::mat4 viewProj
	);

	void Start();
	void Update();

private:
	String m_name;

	std::map<String, GameObject*> m_gameObjects;

	Camera* currentCamera;
	std::map<String, Camera*> m_cameras;
};