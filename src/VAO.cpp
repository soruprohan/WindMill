#include "VAO.h"

// Ask OpenGL for a new, empty VAO. It doesn't know about any data yet -
// that comes later, from LinkAttrib.
VAO::VAO()
{
    glGenVertexArrays(1, &ID);
}

// This function teaches the VAO how to read ONE field out of the Vertex
// struct (like "position" or "normal"). You call it 4 times total in this
// project - once per field - because Vertex has 4 fields.
//
// Example: linking the position field looks like this at the call site:
//   vao.LinkAttrib(vbo, 0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, position));
// In plain words that call means:
//   "Shader slot 0 (position) is made of 3 floats. Each vertex takes up
//    sizeof(Vertex) bytes total, and the position starts right at the
//    beginning of that block (offset 0)."
//
// What the 4 lines below actually do:
void VAO::LinkAttrib(VBO& vbo, GLuint layout, GLint numComponents,
                     GLenum type, GLsizei stride, const void* offset) const
{
    vbo.Bind();   // select the VBO we're reading from

    // Write down the "reading instructions" for this one field.It tells OpenGL exactly how to read an attribute from the VBO.
    glVertexAttribPointer(layout, numComponents, type, GL_FALSE, stride, offset);

    // Turn this slot ON. If you forget this line, the shader just reads
    // zeros for that field no matter what you wrote above.
    glEnableVertexAttribArray(layout);

    vbo.Unbind();   // done reading from it for now
}

// Bind = "make this VAO the active one". Once bound, the GPU already knows
// how to read every field you linked earlier - you don't repeat any of that
// work. Unbind = "stop using it" (pass in 0, OpenGL's way of saying "none").
void VAO::Bind()   const { glBindVertexArray(ID); }
void VAO::Unbind() const { glBindVertexArray(0); }

// Delete frees this VAO from GPU memory. The "if" check just makes it safe
// to call twice by accident - nothing bad happens the second time.
void VAO::Delete()
{
    if (ID != 0) { glDeleteVertexArrays(1, &ID); ID = 0; }
}
