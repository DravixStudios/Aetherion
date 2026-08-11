#pragma once
#include <iostream>
#include "Core/Containers.h" 

class Component {
public:
	Component(String name);

	virtual void Start();
	virtual void Update();
protected:
	String m_name;
};