#pragma once

#include "TextureGeneratorBase.h"
#include <Unvoxeller/FaceRect.h>

namespace Unvoxeller
{
    class AtlasTextureGenerator : public TextureGeneratorBase
    {
    public:
        std::shared_ptr<TextureData> GetTexture(std::vector<FaceRect>& faces, const std::vector<color>& palette,
                                                      const std::vector<vox_model>& models, const bool texturesPOT) override;
    private:
        bool PackFacesIntoAtlas(s32 atlasSize, std::vector<FaceRect>& rects);

        void GenerateAtlasImage(s32 texWidth,
                                s32 texHeight,
                                const std::vector<FaceRect>& faces,
                                const std::vector<vox_model>& models,
                                const std::vector<color>& palette,
                                std::vector<unsigned char>& outImage);


    };
}