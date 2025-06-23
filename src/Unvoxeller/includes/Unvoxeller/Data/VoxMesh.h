#pragma once

#include <string>
#include <vector>
#include <Unvoxeller/Types.h>
#include <Unvoxeller/VoxelTypes.h>

namespace Unvoxeller
{
    struct VoxFace
    {
        std::vector<u32> Indices;
    };

    struct VoxMesh 
    {
        // optional human‐readable name
        std::string Name;

        s32 TextureIndex = 0;
        
        std::vector<vox_vec3> Vertices;
        std::vector<vox_vec3> Normals;
        std::vector<vox_vec2> UVs;

        // std::vector<color> colors;

        // Triangulated faces.
        std::vector<VoxFace> Faces;

        // convenience accessors
        u32 NumVertices() const { return u32(Vertices.size()); }
        u32 NumFaces()    const { return u32(Faces.size()); }
    };
}