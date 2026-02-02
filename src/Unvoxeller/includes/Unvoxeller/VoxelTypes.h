#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <memory>
#include "Types.h"

namespace Unvoxeller
{

	//–– File header (magic + version)
	struct vox_header
	{
		std::string id;
		std::string version;
	};

	//–– Simple 3-dimensional size
	struct vox_size
	{
		s32 x, y, z;
	};

	//–– RGBA color
	struct color
	{
		std::uint8_t r, g, b, a;
	};

	//–– Single voxel record
	struct vox_voxel
	{
		std::uint8_t x, y, z, colorIndex;
	};

	//–– Axis-aligned bounding box
	struct bbox
	{
		f32 minX, minY, minZ;
		f32 maxX, maxY, maxZ;
	};

	//–– One model’s voxel data + lookup grid + bounds
	struct vox_model
	{
		std::vector<vox_voxel>      voxels;
		// pointer to [z][y][x] array, allocated at parse time
		s32*** voxel_3dGrid = nullptr;
		bbox                        boundingBox{
			std::numeric_limits<f32>::max(),
			std::numeric_limits<f32>::max(),
			std::numeric_limits<f32>::max(),
			std::numeric_limits<f32>::lowest(),
			std::numeric_limits<f32>::lowest(),
			std::numeric_limits<f32>::lowest()
		};
	};


	//–– Per‐frame transform attributes
	struct vox_frame_attrib
	{
		s32 frameIndex;
		glm::vec3 translation;
		glm::mat3 rotation;
	};

	//–– Transform node (nTRN chunk)
	struct vox_nTRN
	{
		s32 nodeID;
		std::unordered_map<std::string, std::string> attributes;
		s32 childNodeID;
		s32 layerID;
		s32 framesCount;
		std::vector<vox_frame_attrib> frameAttrib;
		s32 parentNodeID;
	};

	//–– Group node (nGRP chunk)
	struct vox_nGRP
	{
		s32 nodeID;
		std::unordered_map<std::string, std::string> attributes;
		s32 childrenCount;
		std::vector<s32> childrenIDs;
	};

	//–– Single shape‐model reference
	struct vox_nSHP_model
	{
		s32 modelID;
		s32 frameIndex;
	};

	//–– Shape node (nSHP chunk)
	struct vox_nSHP
	{
		s32                                                 nodeID;
		std::unordered_map<std::string, std::string>       attributes;
		std::vector<vox_nSHP_model>                        models;
	};

	//–– Unified material record for MATT & MATL
	struct vox_MATL
	{
		f32 diffuse;
		f32 metal;
		f32 glass;
		f32 emit;
		f32 plastic;
		f32 rough;
		f32 spec;
		f32 ior;
		f32 att;
		f32 flux;
		f32 weight;
	};

	//–– Layer definition (LAYR chunk)
	struct vox_layer
	{
		s32         layerID;
		std::string name;
		bool        hidden;
	};

	//–– All data read from one .vox file
	struct vox_file
	{
		vox_header                         header;
		std::vector<color>                 palette;
		std::vector<vox_size>              sizes;
		std::vector<vox_model>             voxModels;
		std::unordered_map<s32, vox_nTRN>  transforms;
		std::unordered_map<s32, vox_nGRP>  groups;
		std::unordered_map<s32, vox_nSHP>  shapes;
		std::unordered_map<s32, vox_MATL>  materials;
		std::unordered_map<s32, vox_layer> layers;
		bool                               isValid = false;
	};

	// Compose transform (parent first!)
	struct vox_transform
	{
		glm::mat3 rot;
		glm::vec3  trans;
		vox_transform() : rot{ 1,0,0,0,1,0,0,0,1 }, trans{ 0,0,0 } {}
		vox_transform(const glm::mat3& r, const glm::vec3& t) : rot(r), trans(t) {}
	};

	inline vox_transform operator*(const vox_transform& parent, const vox_transform& child) {
		return
		{
			parent.rot * child.rot,              // rotate child by parent
			parent.rot * child.trans + parent.trans // rotate+offset child's translation
		};
	}

}
