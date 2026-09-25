#version 330 core

layout (location = 0) in vec3 aPos; //in means vertex.h info ashbe shader e
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

out vec3 vColor; //out means shader info frag ke dibe
out vec2 vTexCoord;
out vec3 vNormal;

uniform mat4 model; //A uniform is a value supplied by the C++ program to the shader.
uniform mat4 view; //Where is the camera looking from?
uniform mat4 projection; //How should the 3D world be converted into what the camera sees?

void main()
{
    // Right-to-left: the model matrix acts first, projection last.
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    vColor    = aColor;  //Take the color received from the vertex and send it forward.
    vTexCoord = aTexCoord;
    vNormal   = aNormal;   // Phase 9 transforms this by the normal matrix
}
