#pragma once

#include <glm/glm.hpp>

// The one vertex format used by every mesh in the project.
//
// Normals and texture coordinates are filled in from Phase 3 onward even
// though nothing reads them until Phases 8-9. Retrofitting normals into a
// dozen mesh generators later is painful, so they are correct from the start.
struct Vertex
{
    glm::vec3 position; 
    glm::vec3 normal;     // filled from Phase 3, used from Phase 9 (lighting)
    glm::vec2 texCoord;   
    glm::vec3 color;    
};
