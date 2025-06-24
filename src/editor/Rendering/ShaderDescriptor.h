#pragma once
#include <Unvoxeller/Types.h>
#include <vector>

struct ShaderDescriptor
{
	std::vector<u8> Vertex = {};
	std::vector<u8> Fragment = {};
	std::vector<u8> Geometry = {};
};