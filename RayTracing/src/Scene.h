#pragma once

#include<vector>
#include<glm/glm.hpp>

struct Material
{
	glm::vec3 Albedo{ 1.0f };      //颜色
	float Roughness = 1.0f;        //粗糙度
	float Metallic = 0.0f;         //金属度
	glm::vec3 EmissionColor{ 0.0f };     //发光颜色
	float EmissionPower = 0.0f;          //发光强度

	glm::vec3 GetEmission() const { return EmissionColor * EmissionPower; }   //获取发光结果
};

struct Sphere       //球体结构，包括球的半径、位置、应用的材质序号
{
	glm::vec3 Position{ 0.0f };
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct Scene       //场景中包含所有的球体及对应的材质
{
	std::vector<Sphere> Spheres;
	std::vector<Material> Materials;
};