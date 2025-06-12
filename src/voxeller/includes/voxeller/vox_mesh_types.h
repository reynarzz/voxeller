#pragma once
#include "api.h"

#include <vector>
#include <memory>
#include "vox_math_types.h"

enum class mesh_algorithm
{
	VOXEL,
	SMALL_TEXTURE,
	GREEDY_MESHING
};


EXPORT struct VOXELLER_API vox_vertex
{
	std::vector<unsigned int> triangles;
	std::vector<vox_vec3> vertices;

	std::vector<vox_vec3> normals;
	std::vector<vox_vec2> uvs;

	vox_vec4 color;
};

EXPORT struct VOXELLER_API vox_face 
{
	vox_vec3 normal;
	std::vector<int> vertices;
};


EXPORT struct VOXELLER_API vox_material
{
	vox_vec4 albedo;
};

EXPORT struct VOXELLER_API vox_mesh
{
	std::vector<vox_vertex> vertices;
	std::shared_ptr<vox_material> material;
};

EXPORT struct VOXELLER_API vox_scene
{
	std::vector<vox_mesh> meshes;
};

