#pragma once

#include "vox_mesh_types.h"
#include "vox_types.h"

EXPORT class VOXELLER_API vox_mesh_builder 
{
public:
	static std::shared_ptr<vox_scene> build_mesh(const std::shared_ptr<vox_file> vox, const mesh_algorithm algorithm);
	static std::shared_ptr<vox_scene> build_mesh_voxel(const std::shared_ptr<vox_file> vox);
	static std::shared_ptr<vox_scene> build_mesh_small_texture(const std::shared_ptr<vox_file> vox);
	static std::shared_ptr<vox_scene> build_mesh_greedy(const std::shared_ptr<vox_file> vox);
	static void destroy_scene();

private:
	static std::vector<vox_voxel> removeHiddenVoxels(const vox_size& size, const vox_model& model);
};