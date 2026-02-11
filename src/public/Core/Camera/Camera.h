#pragma once
#include <iostream>
#include "Math/Transform.h"
#include "Core/Time.h"

#include "Core/Containers.h" 

class Camera {
public:
	Camera(String name);
	Transform transform;

	virtual void Start();
	virtual void Update();

private:
	String m_name;

protected:
	Time* m_time;
};