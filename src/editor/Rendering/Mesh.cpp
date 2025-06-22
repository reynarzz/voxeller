#include "Mesh.h"
#include <Rendering/RenderingSystem.h>

std::shared_ptr<Mesh> Mesh::CreateMesh(const MeshDescriptor* desc)
{
    return RenderingSystem::GetDevice().lock()->CreateMesh(desc);
}

s32 Mesh::GetIndexCount() const
{
    return IndexCount;
}

s32 Mesh::GetVertexCount() const
{
    return VertexCount;
}

Mesh::~Mesh()
{
   
}