#pragma once
#include <string>
#include <Unvoxeller/api.h>

namespace Unvoxeller
{
    struct UNVOXELLER_API File 
    {
        static std::string GetExecutableFullPath();
        static std::string GetExecutableDir();
        static std::string GetExecutableName();
    };
};