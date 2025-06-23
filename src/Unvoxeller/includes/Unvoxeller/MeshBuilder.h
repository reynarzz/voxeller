#pragma once
#include <vector>
#include <Unvoxeller/FaceRect.h>
#include <Unvoxeller/VoxelTypes.h>
#include <Unvoxeller/Data/UnvoxMesh.h>
#include <memory>


namespace Unvoxeller
{
    class MeshBuilder
    {
    public:
    // Build the actual geometry (vertices and indices) for a mesh from the FaceRect list and a given texture atlas configuration.
    static  std::shared_ptr<UnvoxMesh>  BuildMeshFromFaces(
                const std::vector<FaceRect>& faces,
                int texWidth, int texHeight,
                bool flatShading,
                const std::vector<color>& palette,
                const bbox& box,
                const vox_mat3& rotation  = {},
                const vox_vec3& translation  = {} 
            );
    private:
    };
}