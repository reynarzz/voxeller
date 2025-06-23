#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include "ConvertOptions.h"

namespace Unvoxeller
{
	struct UNVOXELLER_API ExportOptions
	{
		ConvertOptions Converting;
		ModelFormat OutputFormat = ModelFormat::FBX;
	};

}