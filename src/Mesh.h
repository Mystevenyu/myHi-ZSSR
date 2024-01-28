#pragma once
#ifndef MESH_H
#define MESH_H
#include <glad/glad.h> 
#include <vector>
#include <string>
#include "glm/glm.hpp"
#include"Material.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Texture
{
	unsigned int texId;
	std::string texType;
	std::string texPath;
};

class Mesh
{
public:

	//Mesh();
	Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures, const Material& mat)
		: mVertices(vertices), mIndices(indices), mTextures(textures), material(mat), mVAO(0), mVBO(0), mEBO(0) {
		SetMesh();
	}
	//~Mesh();

	bool loadOBJ(const std::string& filename);
	void draw();
	
	std::string name;
	bool mLoaded;
	std::vector<Vertex> mVertices;	GLuint mVBO, mVAO,mEBO;

	void RenderMesh();
	void SetMesh();

private:
	std::vector<unsigned int> mIndices;
	std::vector<Texture> mTextures;
	Material material;

	void initBuffers();
	std::vector<std::string> split(std::string vert, std::string del);
};
#endif //MESH_H

