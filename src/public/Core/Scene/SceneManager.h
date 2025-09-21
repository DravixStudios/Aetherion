#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/Scene/Scene.h"

class SceneManager {
public:
	SceneManager();

	void AddScene(Scene* scene);
	Scene* GetScene(std::string name);

	bool SceneExists(std::string name);

	Scene* GetCurrentScene();

	static SceneManager* GetInstance();
private:
	Scene* m_currentScene;
	std::map<std::string, Scene*> m_scenes;

	static SceneManager* m_instance;
};