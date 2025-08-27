#include "VoxObject.h"

VoxObject::VoxObject(const std::vector<std::shared_ptr<RenderableObject>> &renderables) : _renderables(renderables)
{
}

const std::vector<std::shared_ptr<RenderableObject>> &VoxObject::GetRenderables() const
{
    return _renderables;
}
