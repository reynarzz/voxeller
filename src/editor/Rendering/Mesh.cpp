#include "Mesh.h"

s32 Mesh::GetIndexCount() const
{
    return IndexCount;
}

s32 Mesh::GetVertexCount() const
{
    return VertexCount;
}

PipelineRenderType Mesh::GetPipeline() const
{
    return Pipeline;
}

Mesh::~Mesh()
{
   
}