#pragma once
#include <vector>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/Texture.h>
#include <Rendering/Mesh.h>
#include <Rendering/RenderableTransform.h>

class RenderableObject
{
public:
    RenderableObject();
    
    void SetRenderType(const PipelineRenderType type);
    PipelineRenderType GetRenderType() const;

    void SetDrawType(const RenderDrawType type);
    RenderDrawType GetDrawType() const;

    void SetMesh(std::shared_ptr<Mesh> mesh);
    std::weak_ptr<Mesh> GetMesh() const;
    
    void SetTexture(std::shared_ptr<Texture> texture);
    const std::weak_ptr<Texture> GetTexture() const;
    
    void Destroy();
    bool ShouldDestroy() const;

    RenderableTransform& GetTransform();
    const RenderableTransform& GetTransform() const;
    void SetCanDraw(bool draw);
    bool GetCanDraw() const;

private:
    bool _pendingForDestroy = false;
    bool _canDraw = true;

    RenderableTransform _transform={};
    std::shared_ptr<Mesh> _mesh = nullptr;
    std::shared_ptr<Texture> _texture = {};
    PipelineRenderType _renderType = PipelineRenderType::Opaque_Unlit;
    RenderDrawType _drawType = RenderDrawType::Triangles;
};