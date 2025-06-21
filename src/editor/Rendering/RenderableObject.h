#pragma once
#include <vector>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/Texture.h>
#include <Rendering/Mesh.h>


class RenderableObject
{
public:
    void SetRenderType(const PipelineRenderType type);
    PipelineRenderType GetRenderType() const;

    void SetMesh(std::weak_ptr<Mesh> mesh);
    std::weak_ptr<Mesh> GetMesh() const;
    
    void Destroy();
    bool ShouldDestroy() const;

private:
    bool _pendingForDestroy = false;

    PipelineRenderType Pipeline;
    std::weak_ptr<Mesh> _mesh = {};
    std::vector<std::weak_ptr<Texture>> _textures = {};
    PipelineRenderType _renderType = PipelineRenderType::Opaque;
};