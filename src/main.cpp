#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#define GLEW_STATIC

#include <glad/glad.h> 
#include"Model.h"
#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"
#include"Quad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

#include <assimp/include/assimp/Importer.hpp>
#include <assimp/include/assimp/scene.h>
#include <assimp/include/assimp/postprocess.h>
#include<unordered_map>

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include"stb_image.h"
using namespace std;
// Global Variables
OrbitCamera orbitCamera;
const char* title = "OpenGL Starters";
int pheight = 768;
int pwidth = 1024;
bool gFullScreen = false;
GLFWwindow* pWindow;
bool gWireframe;
bool pan = false;

bool showOpen = false;
float gRadius = 10.0f;
float gYaw = 0.0f;
float gPitch = 0.0f;
double lastmouseX = pwidth / 2.0;
double lastmouseY = pheight / 2.0;

const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 5.0; // units per second
const float MOUSE_SENSITIVITY = 0.1f;



// Function prototypes
void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);

void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void glfw_scrollInput(GLFWwindow* window, double Xoff, double Yoff);
void showFPS(GLFWwindow* window);
bool initOpenGL();

void resetAndPrepareFramebuffer(GLuint fbo, GLuint gNormal, GLuint gPosition, GLuint gAlbedo, GLuint depthMap, int maxLevel, int pwidth, int pheight);
void bindTextureToUnit(GLuint texture, GLenum activeTexture, int unit);
struct Light {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    // Attenuation factors
    float constant;
    float linear;
    float quadratic;
};
Light pointLight;

void setupLights(ShaderProgram& shader) {
    // Setup point lightju
    Light pointLight;
    pointLight.position = glm::vec3(0.0f, 10.0f, 0.0f); // 设定点光源的位置
    pointLight.ambient = glm::vec3(2.5f, 2.5f, 2.5f);  // 环境光参数
    pointLight.diffuse = glm::vec3(2.5f, 2.5f, 2.5f);  // 漫反射参数
    pointLight.specular = glm::vec3(1.5f, 1.5f, 1.5f); // 镜面反射参数

    // Attenuation (衰减) parameters
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    // Set the uniform values for the point light in the shader
    shader.setVec3("pointLight.position", pointLight.position);
    shader.setVec3("pointLight.ambient", pointLight.ambient);
    shader.setVec3("pointLight.diffuse", pointLight.diffuse);
    shader.setVec3("pointLight.specular", pointLight.specular);
    shader.setFloat("pointLight.constant", pointLight.constant);
    shader.setFloat("pointLight.linear", pointLight.linear);
    shader.setFloat("pointLight.quadratic", pointLight.quadratic);
}

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main()
{
	if (!initOpenGL())
	{
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	Model myModel("E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/models/sponza.obj");

	ShaderProgram lightPass("E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/light.vert", "E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/light.frag");
	ShaderProgram hiZPass("E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/hi-Z.vert", "E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/hi-Z.frag");
	ShaderProgram ssrPass("E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/ssr.vert", "E:/Netease/myproj/OpenGL-Renderer-master22(pointlight)lingyigexiangmu/OpenGL-Renderer-master22/shaders/ssr.frag");
	
#pragma endregion
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

#pragma region GBuffer Initialization
	GLuint gPosition;
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, pwidth, pheight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	//位置纹理
	GLuint gNormal;
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, pwidth, pheight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	GLuint gAlbedo;
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pwidth, pheight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, pwidth, pheight, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	// depth texture is gonna be a mipmap so we have to establish the mipmap chain
	glGenerateMipmap(GL_TEXTURE_2D);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 };

	glDrawBuffers(3, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion
	double lastTime = glfwGetTime();
	Quad quad;
	while (!glfwWindowShouldClose(pWindow))
	{
		glm::mat4 model = glm::mat4(1.0f), view;
		showFPS(pWindow);

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightPass.use();
		setupLights(lightPass);
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		// Create the View matrix
		view = orbitCamera.getViewMatrix();
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(orbitCamera.getFOV()), (float)pwidth / (float)pheight, 0.1f, 300.0f);
		lightPass.setMat4("model", model);
		lightPass.setMat4("view", view);
		lightPass.setMat4("projection", projection);

		myModel.DrawModel();

		hiZPass.use();
		glDisable(GL_COLOR_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDepthFunc(GL_ALWAYS);
		int currentWidth = pwidth;
		int currentHeight = pheight;
		int maxDimension = max(pwidth, pheight);
		int maxLevel = 1;

		while (maxDimension > 1) {
			maxDimension /= 2;
			maxLevel++;
		}

		// 在初始化阶段获取uniform变量的位置
		GLuint prevLevelDimensionsLocation = glGetUniformLocation(hiZPass.ID, "prevLevelDimensions");
		GLuint prevLevelLocation = glGetUniformLocation(hiZPass.ID, "prevLevel");


		for (int i = 1; i <= maxLevel; i++) {
			glUniform2i(prevLevelDimensionsLocation, currentWidth, currentHeight);
			glUniform1i(prevLevelLocation, i - 1);

			// update viewport 
			currentWidth = max(1, currentWidth / 2);
			currentHeight = max(1, currentHeight / 2);

			glViewport(0, 0, currentWidth, currentHeight);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, i);
			glBindVertexArray(quad.GetVAO());
			glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, 0);

			
		}

		

		resetAndPrepareFramebuffer(fbo, gNormal, gPosition, gAlbedo, depthMap, maxLevel, pwidth, pheight);

		ssrPass.use();
		ssrPass.setInt("gPosition", 0);
		ssrPass.setInt("gNormal", 1);
		ssrPass.setInt("gAlbedo", 2);
		ssrPass.setInt("depthTexture", 3);
		ssrPass.setMat4("model", model);
		ssrPass.setMat4("view", view);
		ssrPass.setMat4("projection", projection);
		ssrPass.setInt("maxLevel", maxLevel);
		glBindVertexArray(quad.GetVAO());
		glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, 0);


		glfwSwapBuffers(pWindow);
		glfwPollEvents();
	}
	glfwTerminate();

	return 0;
}

bool initOpenGL()
{
	if (!glfwInit())
	{
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	if (gFullScreen)
	{
		GLFWmonitor* pMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* pVmode = glfwGetVideoMode(pMonitor);
		if (pVmode != NULL)
		{
			pWindow = glfwCreateWindow(pVmode->width, pVmode->height, title, pMonitor, NULL);
		}
	}
	else
	{
		pWindow = glfwCreateWindow(pwidth, pheight, title, NULL, NULL);
	}
	if (pWindow == NULL)
	{
		std::cerr << "Failed to create a Window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(pWindow);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
	stbi_set_flip_vertically_on_load(true);

	// Set the required callback functions
	glfwSetKeyCallback(pWindow, glfw_onKey);
	glfwSetFramebufferSizeCallback(pWindow, glfw_onFramebufferSize);
	glfwSetCursorPosCallback(pWindow, glfw_onMouseMove);
	glfwSetScrollCallback(pWindow,glfw_scrollInput);

	// Hides and grabs cursor, unlimited movement
	glfwSetCursorPos(pWindow, pwidth / 2.0, pheight / 2.0);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glViewport(0, 0, pwidth, pheight);

	return true;
}

void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window,GL_TRUE);
	if (key == GLFW_KEY_F && action == GLFW_PRESS)
		gWireframe = !gWireframe;
	if (gWireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); 
	}
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		showOpen = true;
	}
}

void resetAndPrepareFramebuffer(GLuint fbo, GLuint gNormal, GLuint gPosition, GLuint gAlbedo, GLuint depthMap, int maxLevel, int pwidth, int pheight) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gPosition, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);

	glDepthFunc(GL_LEQUAL);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, pwidth, pheight);

	glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers

	// Bind textures to specific texture units
	bindTextureToUnit(gPosition, GL_TEXTURE0, 0);
	bindTextureToUnit(gNormal, GL_TEXTURE1, 1);
	bindTextureToUnit(gAlbedo, GL_TEXTURE2, 2);
	bindTextureToUnit(depthMap, GL_TEXTURE3, 3);
}

void bindTextureToUnit(GLuint texture, GLenum activeTexture, int unit) {
	glActiveTexture(activeTexture);
	glBindTexture(GL_TEXTURE_2D, texture);
}


void glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
	pwidth = width;
	pheight = height;
	glViewport(0, 0, width, height);
}

void showFPS(GLFWwindow* window)
{
	static double prevSec = 0.0;
	static int frameCount = 0;
	double elapsedSec;
	double currentSec = glfwGetTime();

	elapsedSec = currentSec - prevSec;

	//update the FPS 4times in a second
	if (elapsedSec >= 0.25)
	{
		prevSec = currentSec;
		double fps = (double)frameCount / elapsedSec;
		double msPerFrame = 1000.0 / fps;

		std::ostringstream outs;
		outs.precision(3);
		outs << std::fixed
			<< title << " "
			<< "FPS: " << fps << " "
			<< "FrameTime:" << msPerFrame << "(ms)";
		glfwSetWindowTitle(window, outs.str().c_str());
		frameCount = 0;
	}
	frameCount++;
}

void glfw_onMouseMove(GLFWwindow* window, double posX, double posY)
{
	static glm::vec2 lastMousePos = glm::vec2(0, 0);
	if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_MIDDLE)==1)
	{
		// each pixel represents a quarter of a degree rotation (this is our mouse sensitivity)
			gYaw -= ((float)posX - lastMousePos.x) * MOUSE_SENSITIVITY;
			gPitch += ((float)posY - lastMousePos.y) * MOUSE_SENSITIVITY;
	}
	if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_RIGHT) == 1)
	{
		// each pixel represents a quarter of a degree rotation (this is our mouse sensitivity)
		orbitCamera.move(MOUSE_SENSITIVITY*0.5f*glm::vec3((float)posX - lastMousePos.x, (float)posY - lastMousePos.y, 0.0f));
	}


	lastMousePos.x = (float)posX;
	lastMousePos.y = (float)posY;
}


void glfw_scrollInput(GLFWwindow* window, double Xoff, double Yoff)
{
	gRadius -= Yoff;
}
