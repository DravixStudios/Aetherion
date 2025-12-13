#pragma once
#include <iostream>
#include "Math/Transform.h"

class Camera {
public:
	Camera(std::string name);
	Transform transform;

	virtual void Start();
	virtual void Update();
private:
	std::string m_name;
};