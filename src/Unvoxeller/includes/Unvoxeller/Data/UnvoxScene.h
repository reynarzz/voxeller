#pragma once
#include <Unvoxeller/Data/UnvoxMesh.h>

namespace Unvoxeller
{
    struct UnvoxScene
    {
        std::vector<std::shared_ptr<UnvoxMesh>> Meshes;
    };
}