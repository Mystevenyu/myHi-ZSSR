#pragma once
#ifndef MODEL_H
#define MODEL_H
#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/include/assimp/Importer.hpp>
#include <assimp/include/assimp/scene.h>
#include <assimp/include/assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include<unordered_map>
#include <vector>
#include "mesh.h"
#include "ShaderProgram.h"
#include"stb_image.h"

using namespace std;
class Model {
public:
    Assimp::Importer importer;

    std::vector<Texture> texturesStore;
    std::vector<Mesh> meshesStore;
    std::string directory;

	Model(std::string const& path)
	{
		LoadModel(path);
	}


	void DrawModel()
	{
		for (auto& mesh : meshesStore) {
			mesh.RenderMesh();
		}
	}

private:
	
	void LoadModel(std::string const& path)
	{
		const aiScene* scene = ImportScene(path);
		if (!IsValidScene(scene)) return;

		directory = ExtractDirectoryFromPath(path);
		LoadModelNode(scene->mRootNode, scene);
	}

	void LoadModelNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshesStore.push_back(LoadModelMesh(mesh, scene));
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			LoadModelNode(node->mChildren[i], scene);
		}
	}
	bool IsValidScene(const aiScene* scene)
	{
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return false;
		}
		return true;
	}

	const aiScene* ImportScene(const std::string& path)
	{
		return importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_SortByPType |
			aiProcess_PreTransformVertices |
			aiProcess_GenUVCoords |
			aiProcess_OptimizeMeshes);
	}

	Mesh LoadModelMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;
		vertices.reserve(mesh->mNumVertices);
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			glm::vec3 position(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			glm::vec3 normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
			glm::vec2 texCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f, 0.0f);

			vertices.push_back({ position, normal, texCoords });
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			const aiFace& face = mesh->mFaces[i];
			indices.insert(indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
		}

		aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		Material material = LoadModelMaterial(mat);
		vector<Texture> diffuseColorMaps = LoadMaterialTextures(mat, aiTextureType_DIFFUSE);
		textures.insert(textures.end(), diffuseColorMaps.begin(), diffuseColorMaps.end());

		return Mesh(vertices, indices, textures, material);
	}

	Material LoadModelMaterial(aiMaterial* mat)
	{
		Material material;
		aiColor3D color;

		mat->Get(AI_MATKEY_COLOR_AMBIENT, color);
		material.ambientColor = glm::vec4(color.r, color.g, color.b, 1.0f);
		mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		material.diffuseColor = glm::vec4(color.r, color.g, color.b, 1.0f);
		mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
		material.specularColor = glm::vec4(color.r, color.g, color.b, 1.0f);

		return material;
	}

	std::unordered_map<std::string, Texture> textures_loaded_map;
	std::vector<Texture> LoadMaterialTextures(aiMaterial* material, aiTextureType type)
	{
		vector<Texture> textures;
		textures.reserve(material->GetTextureCount(type)); // 预分配内存

		for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
			aiString str;
			material->GetTexture(type, i, &str);
			const char* texturePath = str.C_Str(); // 一次性转换

			auto it = textures_loaded_map.find(texturePath);
			if (it != textures_loaded_map.end()) {
				textures.push_back(it->second);
			}
			else {
				Texture texture = LoadTexture(texturePath, this->directory);
				textures_loaded_map[texturePath] = texture; // 使用 map 存储
				textures.push_back(texture);
			}
		}
		return textures;
	}

	Texture LoadTexture(const char* path, const string& directory) {
		Texture texture;
		texture.texId = TextureFromFile(path, directory);
		texture.texPath = path;
		return texture;
	}

	GLenum DetermineTextureFormat(int nrComponents) {
		switch (nrComponents) {
		case 1: return GL_RED;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: return GL_RGB; // Default format
		}
	}
	void CreateTexture(unsigned int textureID, int width, int height, GLenum format, unsigned char* data) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	std::string ExtractDirectoryFromPath(const std::string& path)
	{
		return path.substr(0, path.find_last_of('/'));
	}

	unsigned int TextureFromFile(const char* path, const std::string& directory)
	{
		std::string filename = directory + '/' + path;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data) {
			GLenum format = DetermineTextureFormat(nrComponents);
			CreateTexture(textureID, width, height, format, data);
			stbi_image_free(data);
		}
		else {
			std::cerr << "Texture failed to load at path: " << filename << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}
};

#endif
