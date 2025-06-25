#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/MeshDescriptor.h>
#include <memory>

class Mesh
{
public:

    Mesh() = default;
    virtual ~Mesh() = 0;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;
    
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;

    static std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc);

protected:
    s32 VertexCount = 0;
    s32 IndexCount = 0;
};