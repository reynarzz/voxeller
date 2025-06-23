#pragma once

#include <Unvoxeller/api.h>
#include "ConvertResults.h"
#include <string>

namespace Unvoxeller 
{
	struct UNVOXELLER_API ExportResults
    {
        ConvertResult Convert = {};
        std::string OutPath = "";
    };
}