#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/MeshDescriptor.h>


class Mesh
{
public:
    static std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc);
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;


protected:
    virtual ~Mesh() = 0;
   
    s32 VertexCount = 0;
    s32 IndexCount = 0;
};