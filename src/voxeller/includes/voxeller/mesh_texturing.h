#pragma once

#include "api.h"
#include "vox_types.h"

EXPORT class VOXELLER_API mesh_texturizer 
{
public:

	static void export_pallete_png(const char* path, const std::vector<color>& pallete);
};