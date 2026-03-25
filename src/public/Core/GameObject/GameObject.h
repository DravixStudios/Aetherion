#pragma once
#include <iostream>

#include "Core/Containers.h"

#include "Core/GameObject/Components/Component.h"
#include "Core/GameObject/Components/Mesh.h"
#include "Math/Transform.h"

class GameObject {
public:
	GameObject(String name);

	String GetName();

	virtual void Start();
	virtual void Update();
	Map<String, Component*> GetComponents();
	void AddComponent(String name, Component* component);

	Transform transform;
private:
	String m_name;
	
	Map<String, Component*> m_components;
};