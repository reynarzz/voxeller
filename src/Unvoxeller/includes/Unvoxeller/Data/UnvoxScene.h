#pragma once
#include <Unvoxeller/Data/UnvoxMesh.h>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Math/VoxMatrix.h>
#include <Unvoxeller/Data/TextureData.h>

namespace Unvoxeller
{
    struct UNVOXELLER_API UnvoxNode
    {
        std::string Name = "";
        std::weak_ptr<UnvoxNode> Parent = {};
        std::vector<std::shared_ptr<UnvoxNode>> Children;
        std::vector<s32> MeshesIndexes;
        vox_mat4 Transform;
    };

    struct UNVOXELLER_API UnvoxMaterial
    {
        std::string Name = "";
        s32 TextureIndex = 0;
    };

    struct UNVOXELLER_API UnvoxScene
    {
        std::string Name = "";
        std::vector<std::shared_ptr<UnvoxMesh>> Meshes;
        std::shared_ptr<UnvoxNode> RootNode = nullptr;
        std::vector<std::shared_ptr<UnvoxMaterial>> Materials;
        std::vector<std::shared_ptr<TextureData>> Textures;
    };
}