#pragma once
#include <Unvoxeller/Data/UnvoxScene.h>
#include <vector>
#include <Unvoxeller/Data/ExportOptions.h>

namespace Unvoxeller
{
    class AssimpSceneWritter
    {
    public:
        bool ExportScenes(const ExportOptions& options, const std::vector<std::shared_ptr<UnvoxScene>> scenes);
    private:
    };
}