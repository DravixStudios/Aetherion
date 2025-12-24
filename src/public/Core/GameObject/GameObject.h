#pragma once
#include <iostream>
#include <map>

#include "Core/GameObject/Components/Component.h"
#include "Core/GameObject/Components/Mesh.h"

class GameObject {
public:
	GameObject(String name);

	String GetName();

	virtual void Start();
	virtual void Update();
	std::map<String, Component*> GetComponents();
	void AddComponent(String name, Component* component);
private:
	String m_name;
	
	std::map<String, Component*> m_components;
};