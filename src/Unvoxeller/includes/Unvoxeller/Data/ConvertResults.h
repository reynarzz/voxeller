#pragma once
#include <vector>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include "MeshInfo.h"
#include "TextureData.h"

namespace Unvoxeller 
{
	enum class ConvertMSG
	{
		FAILED,
		SUCESS,
	};
    
	struct UNVOXELLER_API ConvertResult
	{
		ConvertMSG Msg;
		std::vector<MeshInfo> meshes;

		std::vector<TextureData> Textures;
	};
};

