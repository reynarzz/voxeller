#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>

class Mesh
{
public:
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;

    PipelineRenderType GetPipeline() const;
    
    void Destroy();
    bool ShouldDestroy() const;

protected:
    virtual ~Mesh() = 0;
    bool _pendingForDestroy = false;

    PipelineRenderType Pipeline;
    s32 VertexCount = 0;
    s32 IndexCount = 0;
};