#version 330 core

in vec3 vColor;
in vec2 vTexCoord;
in vec3 vNormal;

out vec4 FragColor;

// Per-object flat colour. Primitives are generated with white vertex colours,
// so one mesh can be redrawn in any colour without regenerating its geometry.
uniform vec3 objectColor;

uniform sampler2D diffuse0;
uniform bool useTexture;   // global toggle, bound to the T key
uniform bool hasTexture;   // set per draw: false for the sun and lamp bulbs

// Slides the texture across the surface. Zero for everything except the
// river and the waterfall, whose water appears to flow because of it.
uniform vec2 uvOffset;

void main()
{
    vec3 base = objectColor;

    if (useTexture && hasTexture)
        base = texture(diffuse0, vTexCoord + uvOffset).rgb;

    // Phases 4-8: no lighting yet. Phase 9 replaces this with Phong.
    FragColor = vec4(base * vColor, 1.0);
}
