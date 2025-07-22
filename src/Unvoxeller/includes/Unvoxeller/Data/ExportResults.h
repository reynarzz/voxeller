#pragma once

#include <Unvoxeller/api.h>
#include "ConvertResults.h"
#include <string>

namespace Unvoxeller 
{
	struct UNVOXELLER_API ExportResults
    {
        ConvertMSG Msg;
        std::string OutPath = "";
    };
}