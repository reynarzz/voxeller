#pragma once
#include <Unvoxeller/Data/TextureData.h>
#include <Unvoxeller/FaceRect.h>
#include <Unvoxeller/VoxelTypes.h>

#include <memory>
#include <vector>

namespace Unvoxeller
{
    class TextureGeneratorBase
    {
    public:
      virtual std::shared_ptr<TextureData> GetTexture(std::vector<FaceRect>& faces, const std::vector<color>& palette,
                                                      const std::vector<vox_model>& models, const bool texturesPOT) = 0;
    private:
    
    };
};