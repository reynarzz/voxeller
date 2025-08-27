#include "Mesh.h"
#include <Rendering/RenderingSystem.h>

std::shared_ptr<Mesh> Mesh::CreateMesh(const MeshDescriptor* desc)
{
    auto mesh = RenderingSystem::GetDevice().lock()->CreateMesh(desc);

    mesh->_vertices = desc->Vertices;
    mesh->_indices = desc->Indices;

    return mesh;
}

s32 Mesh::GetIndexCount() const
{
    return _indices.size();
}

s32 Mesh::GetVertexCount() const
{
    return _vertices.size();
}

std::vector<Vertex>& Mesh::GetVertices() 
{
    return _vertices;
}

std::vector<u32>& Mesh::GetIndices() 
{
    return _indices;
}

Mesh::~Mesh()
{
   
}