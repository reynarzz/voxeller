#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <Voxeller/Math/VoxMath.h>
#include <memory>


namespace Voxeller
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
		s32         frameIndex;
		vox_vec3    translation;
		vox_mat3   rotation;
	};

	//–– Transform node (nTRN chunk)
	struct vox_nTRN
	{
		s32                                                 nodeID;
		std::unordered_map<std::string, std::string>       attributes;
		s32                                                 childNodeID;
		s32                                                 layerID;
		s32                                                 framesCount;
		std::vector<vox_frame_attrib>                      frameAttrib;
	};

	//–– Group node (nGRP chunk)
	struct vox_nGRP
	{
		s32                                                 nodeID;
		std::unordered_map<std::string, std::string>       attributes;
		s32                                                 childrenCount;
		std::vector<s32>                                    childrenIDs;
	};

	//–– Single shape‐model reference
	struct vox_nSHP_model
	{
		s32     modelID;
		s32     frameIndex;
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
		f32   diffuse;
		f32   metal;
		f32   glass;
		f32   emit;
		f32   plastic;
		f32   rough;
		f32   spec;
		f32   ior;
		f32   att;
		f32   flux;
		f32   weight;
	};

	//–– Layer definition (LAYR chunk)
	struct vox_layer
	{
		s32         layerID;
		std::string name;
		bool        hidden;
	};

	//–– All data read from one .vox file
	struct vox_file {
		vox_header                                              header;
		std::vector<color>                                      palette;
		std::vector<vox_size>                                   sizes;
		std::vector<vox_model>                                  voxModels;
		std::unordered_map<s32, vox_nTRN>                       transforms;
		std::unordered_map<s32, vox_nGRP>                       groups;
		std::unordered_map<s32, vox_nSHP>                       shapes;
		std::unordered_map<s32, vox_MATL>                       materials;
		std::unordered_map<s32, vox_layer>                      layers;
		bool                                                    isValid = false;
	};

	// Compose transform (parent first!)
	struct vox_transform
	{
		vox_mat3 rot;
		vox_vec3  trans;
		vox_transform() : rot{ 1,0,0,0,1,0,0,0,1 }, trans{ 0,0,0 } {}
		vox_transform(const vox_mat3& r, const vox_vec3& t) : rot(r), trans(t) {}
	};

	inline vox_transform operator*(const vox_transform& parent, const vox_transform& child) {
		return {
			parent.rot * child.rot,              // rotate child by parent
			parent.rot * child.trans + parent.trans // rotate+offset child's translation
		};
	}

	//–– Parser class declaration
	class VoxParser
	{
	public:
		// Read only header+version (fast check)
		static vox_header                       read_vox_metadata(const char* path);
		static vox_header                       read_vox_metadata(const void* bytes);

		// Full file read (all chunks)
		static std::shared_ptr<vox_file>        read_vox_file(const char* path);

	private:
		// Index into sizes[] when reading multiple models
		static s32                              modelIndex;

		// Default 256-entry MagicaVoxel palette
		static const std::vector<u32>  default_palette;

		// Internal chunk‐parsers
		static void parse_PACK(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_SIZE(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_XYZI(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_RGBA(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_MATT(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_MATL(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_nTRN(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_nGRP(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_nSHP(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
		static void parse_LAYR(std::shared_ptr<vox_file>, std::ifstream&,
			uint32_t contentBytes, uint32_t childrenBytes);
	};

} // namespace Voxeller

