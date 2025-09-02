#pragma once
#include <vector>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include <Unvoxeller/Data/UnvoxScene.h>

namespace Unvoxeller 
{
	enum class ConvertMSG
	{
		FAILED,
		SUCESS,
		ERROR_EMPTY_PATH,
		ERROR_FILE_NOT_FOUND_IN_PATH,
	};
    
	struct UNVOXELLER_API ConvertResult
	{
		ConvertMSG Msg;
		// All the scenes, a frame will be saved in separated scenes.
		std::vector<std::shared_ptr<UnvoxScene>> Scenes;
	};
};