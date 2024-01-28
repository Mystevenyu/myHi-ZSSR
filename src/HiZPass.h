#ifndef HIZPASS_H
#define HIZPASS_H

#include <glad/glad.h>  // ȷ��������OpenGLͷ�ļ�
#include "ShaderProgram.h" 
#include "Quad.h"  // �������һ��quad����ȾHi-Z pass��ȷ���������

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
