#pragma once

#include "api.h"
#include <Voxeller/VoxParser.h>

class VOXELLER_API mesh_texturizer 
{
public:

	static void export_pallete_png(const char* path, const std::vector<Voxeller::color>& pallete);
};