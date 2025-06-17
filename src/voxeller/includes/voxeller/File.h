#pragma once
#include <string>

namespace Voxeller
{
    struct File 
    {
        static std::string GetExecutableFullPath();
        static std::string GetExecutableDir();
        static std::string GetExecutableName();
    };
};