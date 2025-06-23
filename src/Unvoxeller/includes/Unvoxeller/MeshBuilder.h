#pragma once
#include <vector>
#include <Unvoxeller/FaceRect.h>
#include <Unvoxeller/VoxelTypes.h>

#include <assimp/mesh.h> // Remove

namespace Unvoxeller
{
    class MeshBuilder
    {
    public:
    // Build the actual geometry (vertices and indices) for a mesh from the FaceRect list and a given texture atlas configuration.
    static void BuildMeshFromFaces(
                const std::vector<FaceRect>& faces,
                int texWidth, int texHeight,
                bool flatShading,
                const std::vector<color>& palette,
                aiMesh* mesh, 
                const bbox& box,
                const vox_mat3& rotation  = {},
                const vox_vec3& translation  = {} 
            );
    private:
    };
}