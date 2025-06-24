#pragma once
#include <vector>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include "TextureData.h"
#include <Unvoxeller/Data/UnvoxScene.h>

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
		std::shared_ptr<UnvoxScene> Scene;
		std::vector<TextureData> Textures;
	};
};