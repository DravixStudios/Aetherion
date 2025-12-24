#pragma once
#include <iostream>
#include <map>
#include <spdlog/spdlog.h>

#include "Core/Scene/Scene.h"

class SceneManager {
public:
	SceneManager();

	void AddScene(Scene* scene);
	Scene* GetScene(String name);

	bool SceneExists(String name);

	Scene* GetCurrentScene();

	void Start();
	void Update();

	static SceneManager* GetInstance();
private:
	Scene* m_currentScene;
	std::map<String, Scene*> m_scenes;

	static SceneManager* m_instance;
};