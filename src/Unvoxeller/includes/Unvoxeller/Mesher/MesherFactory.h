#pragma once
#include <Unvoxeller/FactoryBase.h>
#include <Unvoxeller/Data/MeshType.h>
#include <Unvoxeller/Mesher/MesherBase.h>
#include <memory>

namespace Unvoxeller
{
    class MesherFactory : public FactoryBase_T<MeshType, std::shared_ptr<MesherBase>>
    {
    public:
        MesherFactory();
    };
}
