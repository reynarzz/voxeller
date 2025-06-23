#include "TextureGeneratorFactory.h"
#include <Unvoxeller/TextureGenerators/AtlasTextureGen.h>

namespace Unvoxeller
{
    TextureGeneratorFactory::TextureGeneratorFactory()
    {
        Elements = 
        {
            { TextureType::Atlas, std::make_shared<AtlasTextureGenerator>() }
        };
    }
}