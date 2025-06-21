#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>

class Mesh
{
public:
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;

protected:
    virtual ~Mesh() = 0;
   
    s32 VertexCount = 0;
    s32 IndexCount = 0;
};