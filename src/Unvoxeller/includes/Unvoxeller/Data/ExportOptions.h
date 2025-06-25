#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include "ConvertOptions.h"
#include <string>

namespace Unvoxeller
{
	struct UNVOXELLER_API ExportOptions
	{
		ConvertOptions Converting;
		ModelFormat OutputFormat = ModelFormat::FBX;
		std::string OutputPath;
	};
}