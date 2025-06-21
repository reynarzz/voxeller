#include "RenderableObject.h"

void RenderableObject::SetRenderType(const PipelineRenderType type)
{
    _renderType = type;
}

void RenderableObject::SetMesh(std::weak_ptr<Mesh> mesh)
{
    _mesh = mesh;
}

std::weak_ptr<Mesh> RenderableObject::GetMesh() const
{
    return _mesh;
}

void RenderableObject::SetTexture(std::weak_ptr<Texture> texture)
{
    _texture = texture;
}

const std::weak_ptr<Texture> RenderableObject::GetTexture() const
{
    return _texture;
}

PipelineRenderType RenderableObject::GetRenderType() const
{
    return _renderType;
}

void RenderableObject::Destroy() 
{
    _pendingForDestroy = true;
}

bool RenderableObject::ShouldDestroy() const
{
    return _pendingForDestroy;
}

RenderableTransform &RenderableObject::GetTransform()
{
    return _transform;
}

const RenderableTransform &RenderableObject::GetTransform() const
{
    return _transform;
}
