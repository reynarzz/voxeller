#pragma once

#include "api.h"
#include <Unvoxeller/VoxParser.h>

class UNVOXELLER_API mesh_texturizer 
{
public:

	static void export_pallete_png(const char* path, const std::vector<Unvoxeller::color>& pallete);
};