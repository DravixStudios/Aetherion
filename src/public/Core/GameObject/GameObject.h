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
private:
	std::string m_name;
	
	std::map<std::string, Component*> m_components;
};