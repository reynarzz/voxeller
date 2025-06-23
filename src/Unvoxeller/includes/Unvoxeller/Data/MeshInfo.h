#pragma once
#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>

namespace Unvoxeller 
{
	
struct UNVOXELLER_API MeshInfo
	{
		s32 Vertices;
		s32 Indices;
		s32 edges;
	};
}