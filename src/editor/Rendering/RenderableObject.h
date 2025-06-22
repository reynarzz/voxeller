#pragma once
#include <vector>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/Texture.h>
#include <Rendering/Mesh.h>
#include <Rendering/RenderableTransform.h>

class RenderableObject
{
public:
    void SetRenderType(const PipelineRenderType type);
    PipelineRenderType GetRenderType() const;

    void SetMesh(std::shared_ptr<Mesh> mesh);
    std::weak_ptr<Mesh> GetMesh() const;
    
    void SetTexture(std::weak_ptr<Texture> texture);
    const std::weak_ptr<Texture> GetTexture() const;
    
    void Destroy();
    bool ShouldDestroy() const;

    RenderableTransform& GetTransform();
    const RenderableTransform& GetTransform() const;

private:
    bool _pendingForDestroy = false;
    
    RenderableTransform _transform={};
    std::shared_ptr<Mesh> _mesh = {};
    std::weak_ptr<Texture> _texture = {};
    PipelineRenderType _renderType = PipelineRenderType::Opaque;
};