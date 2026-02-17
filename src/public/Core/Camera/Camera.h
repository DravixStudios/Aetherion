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

	glm::mat4 GetView() const { return this->m_view; }
	glm::mat4 GetProjection() const { return this->m_projection; }
	glm::mat4 GetViewProjection() const { return this->m_view * this->m_projection; }

private:
	String m_name;

protected:
	Time* m_time;

	glm::mat4 m_view = glm::mat4(1.f);
	glm::mat4 m_projection = glm::mat4(1.f);
};