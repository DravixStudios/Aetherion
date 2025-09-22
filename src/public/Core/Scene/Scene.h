#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/GameObject/GameObject.h"

class Scene {
	friend class SceneManager;
public:
	Scene(std::string name);

	void AddObject(GameObject* object);

	void Start();
	void Update();
private:
	std::string m_name;

	std::map<std::string, GameObject*> m_gameObjects;
};