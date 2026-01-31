#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTex;

out vec2 chTex;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    chTex = inTex;

    gl_Position = uProj * uView * uModel * vec4(inPos, 1.0);
}