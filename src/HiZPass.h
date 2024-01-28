#ifndef HIZPASS_H
#define HIZPASS_H

#include <glad/glad.h>  // 确保包含了OpenGL头文件
#include "ShaderProgram.h" 
#include "Quad.h"  // 如果你用一个quad来渲染Hi-Z pass，确保包括这个

class HiZPass {
public:
    HiZPass(ShaderProgram& shader, GLuint depthMap, int width, int height) :
        hiZShader(shader), depthMap(depthMap), width(width), height(height) {}

    void performHiZPass() {
		hiZPass.use();
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		
    }

private:
    ShaderProgram& hiZShader;
    GLuint depthMap;
    int width, height;
    // Add other necessary members and functions
};


#endif
