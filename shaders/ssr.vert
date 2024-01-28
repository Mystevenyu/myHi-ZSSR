#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexcoords;
layout (location = 3) in vec3 aNormal;
out vec2 texCoords;
out vec3 normals;

 void main()
{   
    gl_Position = vec4(aPos,1.0);
    texCoords = aTexcoords;
    normals = aNormal;
}