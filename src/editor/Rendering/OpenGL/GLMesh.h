#pragma once
#include <Rendering/Mesh.h>
#include <Rendering/MeshDescriptor.h>

// This class contains both index and vertex buffers for simplicity.

class GLMesh : public Mesh
{
public:
    GLMesh(const MeshDescriptor* desc);

    void Bind();
    void Unbind();
private:
    u32 _indexId = 0;
    u32 _varrId = 0;
    u32 _vBuffId = 0;
};