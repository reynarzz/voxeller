#pragma once
#include <string>
#include <voxeller/api.h>

namespace Voxeller
{
    struct VOXELLER_API File 
    {
        static std::string GetExecutableFullPath();
        static std::string GetExecutableDir();
        static std::string GetExecutableName();
    };
};