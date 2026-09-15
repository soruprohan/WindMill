#version 330 core

in vec3 vColor;
in vec2 vTexCoord;
in vec3 vNormal;

out vec4 FragColor;

// Per-object colour. Primitives are generated with white vertex colours, so
// one mesh can be redrawn in any colour without regenerating its geometry.
uniform vec3 objectColor;

void main()
{
    // Phases 4-7: flat placeholder colour, no texture and no lighting yet.
    FragColor = vec4(vColor * objectColor, 1.0);
}
