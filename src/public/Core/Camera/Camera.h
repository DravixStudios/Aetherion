#pragma once
#include <iostream>
#include "Math/Transform.h"
#include "Core/Time.h"

class Camera {
public:
	Camera(std::string name);
	Transform transform;

	virtual void Start();
	virtual void Update();

private:
	std::string m_name;

protected:
	Time* m_time;
};