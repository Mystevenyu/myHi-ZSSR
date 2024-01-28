#version 330 core

uniform sampler2D depthBuffer;
uniform int prevLevel;

in vec2 TexCoord;

out float fragDepth;

void main(void) {
    // current level texCoord
    vec2 curLevelTexelSize = 1.0 / vec2(textureSize(depthBuffer, prevLevel));
    vec2 curLevelTexelCoord = gl_FragCoord.xy * curLevelTexelSize;

    // Get the depth value of the current level and the previous level
    float depthCurrent = textureLod(depthBuffer, TexCoord, prevLevel).r;
    float depthNeighbor1 = textureLod(depthBuffer, TexCoord + vec2(curLevelTexelSize.x, 0.0), prevLevel).r;
    float depthNeighbor2 = textureLod(depthBuffer, TexCoord + vec2(0.0, curLevelTexelSize.y), prevLevel).r;

    float minDepth = min(depthCurrent, min(depthNeighbor1, depthNeighbor2));

    gl_FragDepth = minDepth;
}
