#pragma once
#include <iostream>
#include <map>

#include "Core/GameObject/Components/Component.h"
#include "Core/GameObject/Components/Mesh.h"

class GameObject {
public:
	GameObject(std::string name);

	std::string GetName();

	virtual void Start();
	virtual void Update();
	std::map<std::string, Component*> GetComponents();
	void AddComponent(std::string name, Component* component);
private:
	std::string m_name;
	
	std::map<std::string, Component*> m_components;
};