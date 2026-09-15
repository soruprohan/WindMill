#version 330 core

in vec3 vColor;
in vec2 vTexCoord;
in vec3 vNormal;

out vec4 FragColor;

void main()
{
    // Phases 3-7: flat placeholder colour, no texture and no lighting yet.
    FragColor = vec4(vColor, 1.0);
}
