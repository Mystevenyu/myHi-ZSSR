#pragma once
#include<glm/glm.hpp>
struct Material {
	//材质颜色光照
	glm::vec4 ambientColor;
	//漫反射
	glm::vec4 diffuseColor;
	//镜反射
	glm::vec4 specularColor;
};