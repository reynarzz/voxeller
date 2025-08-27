#pragma once
#include <vector>
#include "RenderableObject.h"

class VoxObject
{
public:
    VoxObject(const std::vector<std::shared_ptr<RenderableObject>>& renderables);
    const std::vector<std::shared_ptr<RenderableObject>>& GetRenderables() const;
    
private:
    std::vector<std::shared_ptr<RenderableObject>> _renderables;
};