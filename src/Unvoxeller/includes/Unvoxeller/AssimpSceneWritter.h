#pragma once
#include <Unvoxeller/Data/UnvoxScene.h>
#include <Unvoxeller/Data/ExportOptions.h>
#include <vector>

namespace Unvoxeller
{
    class AssimpSceneWritter
    {
    public:
        bool ExportScenes(const ExportOptions& options, const std::vector<std::shared_ptr<UnvoxScene>> scenes);
    private:
    };
}