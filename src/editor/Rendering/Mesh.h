#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>

class Mesh
{
public:
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;

    PipelineRenderType GetPipeline() const;

protected:
    virtual ~Mesh() = 0;

    PipelineRenderType Pipeline;
    s32 VertexCount;
    s32 IndexCount;
};