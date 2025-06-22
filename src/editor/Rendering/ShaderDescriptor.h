#pragma once
#include <Unvoxeller/Types.h>
#include <vector>

struct ShaderDescriptor
{
	std::vector<u8> Vertex;
	std::vector<const u8> Fragment;
};
