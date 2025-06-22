#pragma once
#include <vector>
#include <Unvoxeller/Types.h>
#include <Unvoxeller/Math/VoxVector.h>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/MeshRenderType.h>


struct Vertex
{
    Unvoxeller::vox_vec3 Position;
    Unvoxeller::vox_vec3 Normal;
    Unvoxeller::vox_vec2 UV;
};

struct MeshDescriptor
{
    MeshRenderType RenderType;
    std::vector<Vertex> Vertices;
    std::vector<u32> Indices;
};
