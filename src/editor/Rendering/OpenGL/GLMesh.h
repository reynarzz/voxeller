#pragma once
#include <Rendering/Mesh.h>
#include <Rendering/MeshDescriptor.h>

// This class contains both index and vertex buffers for simplicity, no need to divide (which is what we should do in larger projects)
class GLMesh : public Mesh
{
public:
    GLMesh(const MeshDescriptor* desc);
    ~GLMesh();

    void Bind() const;
    void Unbind() const;
private:
    u32 _ibo = 0;
    u32 _vao = 0;
    u32 _vbo = 0;
};