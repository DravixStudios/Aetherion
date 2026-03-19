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

	void SetDimensions(uint32_t nWidth, uint32_t nHeight);

	static SceneManager* GetInstance();
private:
	Scene* m_currentScene;
	Map<String, Scene*> m_scenes;

	static SceneManager* m_instance;
};