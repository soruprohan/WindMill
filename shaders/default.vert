#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 vColor;
out vec2 vTexCoord;
out vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // Right-to-left: the model matrix acts first, projection last.
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    vColor    = aColor;
    vTexCoord = aTexCoord;
    vNormal   = aNormal;   // Phase 9 transforms this by the normal matrix
}
