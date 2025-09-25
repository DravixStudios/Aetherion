#pragma once
#include <iostream>

class Component {
public:
	Component(std::string name);

	virtual void Start();
	virtual void Update();
protected:
	std::string m_name;
};