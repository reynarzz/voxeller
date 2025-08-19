#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include <string>

namespace Unvoxeller
{
	enum class ModelFormat
	{
		FBX, OBJ, GLB, GLTF, DAE
	};

	struct UNVOXELLER_API ExportOptions
	{
		std::string InputPath;
		std::string OutputDir;
		std::string OutputName;
		ModelFormat OutputFormat = ModelFormat::FBX;
	};
}