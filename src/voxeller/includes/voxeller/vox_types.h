#pragma once
#include "api.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "vox_math_types.h"

EXPORT struct VOXELLER_API vox_header
{
    std::string id;
    std::string version;
};

EXPORT struct VOXELLER_API vox_voxel 
{
    int x, y, z;
    int colorIndex;
};

EXPORT struct VOXELLER_API color
{
    char r, g, b, a;
};

EXPORT struct VOXELLER_API vox_size 
{
    int x, y, z;
};

EXPORT struct VOXELLER_API vox_nSHP_model
{
    int modelID;
    std::unordered_map<std::string, std::string> attributes_reserved;
    float frameIndex; // starts from 0
};

EXPORT struct VOXELLER_API vox_frame_attrib
{
    vox_imat3 rotation;
    vox_vec3 translation;
    int frameIndex;
};

EXPORT struct VOXELLER_API vox_nTRN
{
    int nodeID;
    std::unordered_map<std::string, std::string> attributes;
    int childNodeID;
    int reservedID;
    int layerID;
    int framesCount;
    std::vector<vox_frame_attrib> frameAttrib;

};

EXPORT struct VOXELLER_API vox_bounding_box
{
    float minX, minY, minZ,
        maxX, maxY, maxZ;
};

EXPORT struct VOXELLER_API vox_model
{
    std::string name;
    int*** voxel_3dGrid; // int is the color index
    vox_bounding_box boundingBox;
    std::vector<vox_voxel> voxels;
};


EXPORT struct VOXELLER_API vox_nGRP
{
    int nodeID;
    std::unordered_map<std::string, std::string> attributes;
    int childrenCount;
    std::vector<int> childrenIDs;
};

EXPORT struct VOXELLER_API vox_nSHP
{
    int nodeID;
    std::unordered_map<std::string, float> frames;
    int modelsCount;
    std::vector<vox_nSHP_model> models;
};

EXPORT struct VOXELLER_API vox_MATL 
{
    float diffuse;
    float metal;
    float glass;
    float emit;

    float weight;
    float rough;
    float spec;
    float ior;
    float att;
    float flux;
    float plastic;
};


EXPORT struct VOXELLER_API vox_file
{
    std::string name;
    vox_header header;
    std::vector<vox_size> sizes;

    //std::vector<vox_model> models; // TODO: use this instead
    std::vector<vox_model> voxModels; // remove
    std::vector<color> pallete;
    std::unordered_map<int, vox_nGRP> groups;
    std::unordered_map<int, vox_nSHP> shapes;
    std::unordered_map<int, vox_nTRN> transforms;
    std::unordered_map<int, vox_MATL> materials;


    bool isValid;
};

