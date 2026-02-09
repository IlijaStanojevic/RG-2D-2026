#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTex;

out vec2 chTex;
out vec3 chFragPos;
out vec3 chNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    chTex = inTex;

    // world position
    vec4 worldPos = uModel * vec4(inPos, 1.0);
    chFragPos = worldPos.xyz;

    // world normal (flat plane). Use normal matrix anyway (safe if you rotate/scale)
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    chNormal = normalize(normalMatrix * vec3(0.0, 1.0, 0.0)); // plane facing +Y

    gl_Position = uProj * uView * worldPos;
}
