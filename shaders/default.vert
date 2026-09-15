#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 vColor;

// Phase 3 adds: uniform mat4 model, view, projection.

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
