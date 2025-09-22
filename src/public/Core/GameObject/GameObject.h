#pragma once
#include <iostream>

class GameObject {
public:
	GameObject(std::string name);

	std::string GetName();

	virtual void Start();
	virtual void Update();
private:
	std::string m_name;
};