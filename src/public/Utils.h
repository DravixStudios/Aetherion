#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct WVP {
	glm::mat4 World;
	glm::mat4 View;
	glm::mat4 Projection;
};