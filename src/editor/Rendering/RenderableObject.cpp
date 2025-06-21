#include "RenderableObject.h"

void RenderableObject::SetRenderType(const PipelineRenderType type)
{
    _renderType = type;
}

PipelineRenderType RenderableObject::GetRenderType() const
{
    return _renderType;
}

void RenderableObject::SetMesh(std::weak_ptr<Mesh> mesh)
{
    _mesh = mesh;
}

std::weak_ptr<Mesh> RenderableObject::GetMesh() const
{
    return _mesh;
}

void RenderableObject::Destroy() 
{
    _pendingForDestroy = true;
}

bool RenderableObject::ShouldDestroy() const
{
    return _pendingForDestroy;
}

