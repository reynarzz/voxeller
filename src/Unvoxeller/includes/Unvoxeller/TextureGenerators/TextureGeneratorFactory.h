#pragma once
#include <Unvoxeller/FactoryBase.h>
#include <Unvoxeller/Data/TextureType.h>
#include <Unvoxeller/TextureGenerators/TextureGeneratorBase.h>

namespace Unvoxeller
{
    class TextureGeneratorFactory : public FactoryBase<TextureType, std::shared_ptr<TextureGeneratorBase>>
    {
    public:
        TextureGeneratorFactory();

    };
}