#version 330 core
layout(location = 0) in vec2 aPos;   // fullscreen quad pos
layout(location = 1) in vec2 aUV;    // 0..1

out vec2 chTex;

uniform float uCamX;  // UV space (0..1)
uniform float uCamY;
uniform float uViewW;
uniform float uViewH;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);

    float uStart = uCamX - uViewW * 0.5;
    float vStart = uCamY - uViewH * 0.5;

    chTex = vec2(
        uStart + aUV.x * uViewW,
        vStart + aUV.y * uViewH
    );
}
