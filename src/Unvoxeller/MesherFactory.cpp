#include <Unvoxeller/Mesher/MesherFactory.h>
#include <Unvoxeller/Mesher/GreedyMesher.h>
#include <Unvoxeller/Mesher/PaletteMesher.h>
#include <Unvoxeller/Mesher/VoxelLikeMesher.h>

namespace Unvoxeller
{
    MesherFactory::MesherFactory()
    {
        Elements = 
        {
            { MeshType::Greedy, std::make_shared<GreedyMesher>() },
            { MeshType::Voxel, std::make_shared<VoxelLikeMesher>() },

        };
    }
}

