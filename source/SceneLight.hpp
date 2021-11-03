#pragma once

#include <glm/glm.hpp>

struct SceneLight
{
	glm::vec3 dir;
	glm::vec3 intensity;
};

struct ShaderLight
{
	glm::vec3 pos;
	float pad_0;
	glm::vec3 dir;
	float pad_1;
	glm::vec3 intensity;
};

struct LightBuffer
{
	ShaderLight lights[16];
	uint32 count;
};