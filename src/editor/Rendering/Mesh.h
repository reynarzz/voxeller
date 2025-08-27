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

    std::vector<Vertex>& GetVertices();
    std::vector<u32>& GetIndices();
    static std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc);

protected:
    std::vector<Vertex> _vertices;
    std::vector<u32> _indices;
};