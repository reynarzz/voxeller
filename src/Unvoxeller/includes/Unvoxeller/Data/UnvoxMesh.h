#pragma once

#include <string>
#include <vector>
#include <Unvoxeller/Types.h>
#include <Unvoxeller/VoxelTypes.h>
#include <Unvoxeller/api.h>

namespace Unvoxeller
{
    struct UNVOXELLER_API UnvoxFace
    {
        std::vector<u32> Indices;
    };

    struct UNVOXELLER_API UnvoxMesh 
    {
        std::string Name;

        s32 MaterialIndex = 0;
        
        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec2> UVs;

        std::vector<UnvoxFace> Faces;
    };
}