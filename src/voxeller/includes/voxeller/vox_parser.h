#pragma once
#include "api.h"

#include "vox_types.h"
#include <memory>

EXPORT class VOXELLER_API vox_parser
{
public:

    static bool isValid(const char* path);

    static std::shared_ptr<vox_file> read_vox_file(const char* path);
    static vox_header read_vox_metadata(const char* path);
    static vox_header read_vox_metadata(const void* bytes);
    
    const static std::vector<unsigned int> default_pallete;

private:
    static void parse_SIZE(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_XYZI(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_RGBA(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_PACK(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_nTRN(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_nGRP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_nSHP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);
    static void parse_MATL(std::shared_ptr<vox_file> vox, std::ifstream& voxFile);

    static int modelIndex;
};