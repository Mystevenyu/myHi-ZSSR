#version 330 core
layout(location = 0) out vec3 gNormal;
layout(location = 1) out vec3 gPosition;
layout(location = 2) out vec3 gColor;

uniform vec3 lightPos;
uniform mat4 view;

uniform sampler2D diffuseMap;

in vec3 worldPos;
in vec3 oNormal;
in vec2 texCoord;


void main()
{   
    gNormal = oNormal;
    gPosition = worldPos;
    vec3 lightDir = normalize(vec3(view * vec4(lightPos, 1.0)) - worldPos);
    gColor = texture(diffuseMap, texCoord).rgb * dot(lightDir, oNormal);
}
