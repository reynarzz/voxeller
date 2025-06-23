#pragma once
#include <Unvoxeller/VoxelTypes.h>

namespace Unvoxeller
{


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

}; // namespace Unvoxeller
