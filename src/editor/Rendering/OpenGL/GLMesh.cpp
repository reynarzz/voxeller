#include "GLMesh.h"
#include <Rendering/OpenGL/GLInclude.h>


GLMesh::GLMesh(const MeshDescriptor *desc)
{
    VertexCount = desc->Vertices.size();
    IndexCount = desc->Indices.size();
    
    GL_CALL(glGenVertexArrays(1, &_vao));
    GL_CALL(glBindVertexArray(_vao));

    GL_CALL(glGenBuffers(1, &_vbo));
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, _vbo));

    GL_CALL(glBufferData(GL_ARRAY_BUFFER, desc->Vertices.size() * sizeof(Vertex), desc->Vertices.data(), GL_STATIC_DRAW));

    GL_CALL(glGenBuffers(1, &_ibo));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo));
    GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, desc->Indices.size() * sizeof(u32), desc->Indices.data(), GL_STATIC_DRAW));

    constexpr u32 stride = sizeof(Vertex);
    constexpr s32 vertexLayoutIndex = 0;
    constexpr s32 normalLayoutIndex = 1;
    constexpr s32 uvLayoutIndex = 2;
    
    // Vertex position
    GL_CALL(glEnableVertexAttribArray(vertexLayoutIndex));
    GL_CALL(glVertexAttribPointer(vertexLayoutIndex, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, Position)));

    // Normals
    GL_CALL(glEnableVertexAttribArray(normalLayoutIndex));
    GL_CALL(glVertexAttribPointer(normalLayoutIndex, 3, GL_FLOAT, GL_FALSE, stride,  (void*)offsetof(Vertex, Normal)));

    // UV
    GL_CALL(glEnableVertexAttribArray(uvLayoutIndex));
    GL_CALL(glVertexAttribPointer(uvLayoutIndex, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, UV)));

    GL_CALL(glBindVertexArray(0));
}

void GLMesh::Bind() const
{
    GL_CALL(glBindVertexArray(_vao));
}

void GLMesh::Unbind() const
{
   GL_CALL(glBindVertexArray(0));
}

GLMesh::~GLMesh()
{
    Unbind();
    GL_CALL(glDeleteBuffers(1, &_vbo));
    GL_CALL(glDeleteBuffers(1, &_ibo));
    GL_CALL(glDeleteVertexArrays(1, &_vao));
}