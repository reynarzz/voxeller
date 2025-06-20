#pragma once
#include <vector>
#include <Unvoxeller/Types.h>

struct Vertex
{
    f32 x, y, z;
    f32 nx, ny, nz;
    f32 u, v;
};

struct MeshDescriptor
{
    std::vector<Vertex> Vertices;
    std::vector<u32> Indices;
};
