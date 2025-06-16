#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <limits>
#include <fstream>
#include <voxeller/VoxMath.h>

namespace Voxeller 
{

//–– File header (magic + version)
struct vox_header {
    std::string id;
    std::string version;
};

//–– Simple 3-dimensional size
struct vox_size
{
    int x, y, z;
};

//–– RGBA color
struct color {
    std::uint8_t r, g, b, a;
};

//–– Single voxel record
struct vox_voxel {
    std::uint8_t x, y, z, colorIndex;
};

//–– Axis-aligned bounding box
struct bbox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
};

//–– One model’s voxel data + lookup grid + bounds
struct vox_model {
    std::vector<vox_voxel>      voxels;
    // pointer to [z][y][x] array, allocated at parse time
    int***                      voxel_3dGrid = nullptr;
    bbox                        boundingBox {
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()
    };
};


//–– Per‐frame transform attributes
struct vox_frame_attrib {
    int         frameIndex;
    vox_vec3    translation;
    vox_mat3   rotation;
};

//–– Transform node (nTRN chunk)
struct vox_nTRN {
    int                                                 nodeID;
    std::unordered_map<std::string, std::string>       attributes;
    int                                                 childNodeID;
    int                                                 layerID;
    int                                                 framesCount;
    std::vector<vox_frame_attrib>                      frameAttrib;
};

//–– Group node (nGRP chunk)
struct vox_nGRP {
    int                                                 nodeID;
    std::unordered_map<std::string, std::string>       attributes;
    int                                                 childrenCount;
    std::vector<int>                                    childrenIDs;
};

//–– Single shape‐model reference
struct vox_nSHP_model {
    int     modelID;
    int     frameIndex;
};

//–– Shape node (nSHP chunk)
struct vox_nSHP {
    int                                                 nodeID;
    std::unordered_map<std::string, std::string>       attributes;
    std::vector<vox_nSHP_model>                        models;
};

//–– Unified material record for MATT & MATL
struct vox_MATL {
    float   diffuse;
    float   metal;
    float   glass;
    float   emit;
    float   plastic;
    float   rough;
    float   spec;
    float   ior;
    float   att;
    float   flux;
    float   weight;
};

//–– Layer definition (LAYR chunk)
struct vox_layer {
    int         layerID;
    std::string name;
    bool        hidden;
};

//–– All data read from one .vox file
struct vox_file {
    vox_header                                              header;
    std::vector<color>                                      palette;
    std::vector<vox_size>                                   sizes;
    std::vector<vox_model>                                  voxModels;
    std::unordered_map<int, vox_nTRN>                       transforms;
    std::unordered_map<int, vox_nGRP>                       groups;
    std::unordered_map<int, vox_nSHP>                       shapes;
    std::unordered_map<int, vox_MATL>                       materials;
    std::unordered_map<int, vox_layer>                      layers;
    bool                                                    isValid = false;
};

// Compose transform (parent first!)
struct vox_transform {
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
class VoxParser {
public:
    // Read only header+version (fast check)
    static vox_header                       read_vox_metadata(const char *path);
    static vox_header                       read_vox_metadata(const void *bytes);

    // Full file read (all chunks)
    static std::shared_ptr<vox_file>        read_vox_file(const char *path);

private:
    // Index into sizes[] when reading multiple models
    static int                              modelIndex;

    // Default 256-entry MagicaVoxel palette
    static const std::vector<unsigned int>  default_palette;

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

} // namespace voxeller

