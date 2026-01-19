#pragma once

#include <glm/glm.hpp>

struct Ray      //光线结构，包含光线的起点、方向
{
	glm::vec3 Origin;
	glm::vec3 Direction;
};
