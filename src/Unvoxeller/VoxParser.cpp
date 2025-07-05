

#include <Unvoxeller/VoxParser.h>

#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <memory>
#include <cstdint>

namespace Unvoxeller
{
    int VoxParser::modelIndex = 0;
    
    vox_header VoxParser::read_vox_metadata(const char* path) 
    {
        std::ifstream voxFile(path, std::ios::binary);
        vox_header header{};
        modelIndex = 0;
        if (voxFile.is_open()) 
        {
            char magic[4];
            int version;
            voxFile.read(magic, 4);
            voxFile.read(reinterpret_cast<char*>(&version), 4);
            std::string magicStr(magic, 4);
            header.id = magicStr;
            header.version = std::to_string(version);
        }
        else 
        {
            std::cerr << "Invalid file path: " << path << '\n';
        }
        return header;
    }

    vox_header VoxParser::read_vox_metadata(const void* bytes) 
    {
        // Not implemented: reading from a memory buffer
        return {};
    }

    std::shared_ptr<vox_file> VoxParser::read_vox_file(const char* path) 
    {
        std::ifstream voxFile(path, std::ios::binary);
        if (!voxFile.is_open()) 
        {
            std::cerr << "Invalid file path: " << path << '\n';
            return nullptr;
        }

        // Prepare vox_file structure
        std::shared_ptr<vox_file> vox = std::make_shared<vox_file>();
        vox->header = {};
        vox->palette.clear();
        vox->sizes.clear();
        vox->voxModels.clear();
        vox->transforms.clear();
        vox->groups.clear();
        vox->shapes.clear();
        vox->materials.clear();
        vox->layers.clear();  // assuming vox_file has a container for layers

        vox->palette.resize(default_palette.size());
        for (size_t i = 0; i < default_palette.size(); ++i) 
        {
            uint32_t c = default_palette[i];
            vox->palette[i].r = uint8_t((c >> 0) & 0xFF);
            vox->palette[i].g = uint8_t((c >> 8) & 0xFF);
            vox->palette[i].b = uint8_t((c >> 16) & 0xFF);
            vox->palette[i].a = uint8_t((c >> 24) & 0xFF);
        }
        
        // track if we ever see an RGBA chunk
        bool sawRGBA = false;
        // Read file magic and version
        char magic[4];
        int version;
        voxFile.read(magic, 4);
        voxFile.read(reinterpret_cast<char*>(&version), 4);
        std::string magicStr(magic, 4);
        if (magicStr != "VOX ") 
        {
            std::cerr << "Invalid .vox file format (magic number not found)\n";
            return vox;
        }
        vox->header.id = magicStr;
        vox->header.version = std::to_string(version);

        // Read MAIN chunk header
        char mainChunkId[4];
        voxFile.read(mainChunkId, 4);
        if (std::strncmp(mainChunkId, "MAIN", 4) != 0) {
            std::cerr << "Invalid .vox file: MAIN chunk not found\n";
            return vox;
        }
        // Read MAIN chunk sizes (content and children)
        uint32_t mainContentBytes = 0;
        uint32_t mainChildrenBytes = 0;
        voxFile.read(reinterpret_cast<char*>(&mainContentBytes), 4);
        voxFile.read(reinterpret_cast<char*>(&mainChildrenBytes), 4);
        // mainContentBytes is expected to be 0 for MAIN, children size is the rest of the file

        // Loop through all chunks inside MAIN
        modelIndex = 0;
        while (true) {
            // Read the next chunk ID
            char chunkId[4];
            if (!voxFile.read(chunkId, 4)) {
                // Reached end of file or error
                break;
            }
            uint32_t chunkContentBytes = 0;
            uint32_t chunkChildrenBytes = 0;
            voxFile.read(reinterpret_cast<char*>(&chunkContentBytes), 4);
            voxFile.read(reinterpret_cast<char*>(&chunkChildrenBytes), 4);
            std::string chunkStr(chunkId, 4);



            if (chunkStr == "PACK") {
                // Multiple models chunk
                parse_PACK(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "SIZE") {
                parse_SIZE(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "XYZI") {
                parse_XYZI(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "RGBA") {
                sawRGBA = true;
                parse_RGBA(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "MATT") {
                parse_MATT(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "MATL") {
                parse_MATL(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "nTRN") {
                parse_nTRN(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "nGRP") {
                parse_nGRP(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "nSHP") {
                parse_nSHP(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else if (chunkStr == "LAYR") {
                parse_LAYR(vox, voxFile, chunkContentBytes, chunkChildrenBytes);
            }
            else {
                // Unknown chunk type: skip it
                voxFile.seekg(chunkContentBytes + chunkChildrenBytes, std::ios::cur);
            }
        }

        // If no RGBA chunk was encountered, use default palette
        if (!sawRGBA) {
            vox->palette.resize(default_palette.size());
            for (size_t i = 0; i < default_palette.size(); ++i) {
                uint32_t c = default_palette[i];
                vox->palette[i] = {
                    uint8_t(c & 0xFF),
                    uint8_t((c >> 8) & 0xFF),
                    uint8_t((c >> 16) & 0xFF),
                    uint8_t((c >> 24) & 0xFF)
                };
            }
        }

        vox->isValid = true;
        return vox;
    }

    void VoxParser::parse_PACK(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // The PACK chunk contains one int: number of models
        if (contentBytes < 4) {
            std::cerr << "Invalid PACK chunk size\n";
            // Skip anyway
        }
        uint32_t numModels = 0;
        voxFile.read(reinterpret_cast<char*>(&numModels), 4);
        // We could store numModels if needed, but it�s mainly for verification.
        // Skip any child chunks bytes (should be none for PACK)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_SIZE(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // SIZE chunk: 3 ints (x, y, z dimensions)
        if (contentBytes != 12) {
            // If contentBytes is not 12, skip chunk
            std::cerr << "Unexpected SIZE chunk length: " << contentBytes << '\n';
        }
        vox_size size;
        voxFile.read(reinterpret_cast<char*>(&size.x), 4);
        voxFile.read(reinterpret_cast<char*>(&size.y), 4);
        voxFile.read(reinterpret_cast<char*>(&size.z), 4);
        vox->sizes.push_back(size);
        // Skip any children (SIZE normally has no children)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
        // (Optional) Debug: print model dimensions
        // std::cout << "Model " << vox->sizes.size()-1 << " size: " 
        //           << size.x << " x " << size.y << " x " << size.z << "\n";
    }

    void VoxParser::parse_XYZI(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // XYZI chunk: int N (number of voxels), then N * 4 bytes (x,y,z,colorIndex)
        uint32_t numVoxels = 0;
        voxFile.read(reinterpret_cast<char*>(&numVoxels), 4);
        vox_model model;
        model.voxels.resize(numVoxels);

        // Initialize a 3D grid for this model for quick lookup (filled with -1)
        const vox_size& size = vox->sizes[modelIndex++];
        int sx = size.x, sy = size.y, sz = size.z;
        model.voxel_3dGrid = new int** [sz];
        for (int z = 0; z < sz; ++z) {
            model.voxel_3dGrid[z] = new int* [sy];
            for (int y = 0; y < sy; ++y) {
                model.voxel_3dGrid[z][y] = new int[sx];
                std::fill(model.voxel_3dGrid[z][y], model.voxel_3dGrid[z][y] + sx, -1);
            }
        }

        // Read each voxel
        for (uint32_t i = 0; i < numVoxels; ++i) {
            vox_voxel voxData;
            uint8_t xi, yi, zi, ci;
            voxFile.read(reinterpret_cast<char*>(&xi), 1);
            voxFile.read(reinterpret_cast<char*>(&yi), 1);
            voxFile.read(reinterpret_cast<char*>(&zi), 1);
            voxFile.read(reinterpret_cast<char*>(&ci), 1);
            voxData.x = xi;
            voxData.y = yi;
            voxData.z = zi;
            voxData.colorIndex = ci;
            model.voxels[i] = voxData;
            // Mark voxel in 3D grid and update model bounding box
            model.voxel_3dGrid[zi][yi][xi] = static_cast<int>(i);
            model.boundingBox.minX = std::min(model.boundingBox.minX, static_cast<float>(xi));
            model.boundingBox.minY = std::min(model.boundingBox.minY, static_cast<float>(yi));
            model.boundingBox.minZ = std::min(model.boundingBox.minZ, static_cast<float>(zi));
            model.boundingBox.maxX = std::max(model.boundingBox.maxX, static_cast<float>(xi));
            model.boundingBox.maxY = std::max(model.boundingBox.maxY, static_cast<float>(yi));
            model.boundingBox.maxZ = std::max(model.boundingBox.maxZ, static_cast<float>(zi));
        }

        vox->voxModels.push_back(std::move(model));
        // Skip any children chunks (XYZI typically has none)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_RGBA(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // RGBA chunk: 256 * 4 bytes = 1024 bytes (even though palette index 0 is unused)
        const size_t paletteSize = 256;
        vox->palette.resize(paletteSize);
        if (contentBytes != paletteSize * 4) {
            // If there's a discrepancy, adjust reading to smaller of expected vs actual
        }
        for (size_t i = 0; i < paletteSize; ++i) {
            uint8_t r, g, b, a;
            voxFile.read(reinterpret_cast<char*>(&r), 1);
            voxFile.read(reinterpret_cast<char*>(&g), 1);
            voxFile.read(reinterpret_cast<char*>(&b), 1);
            voxFile.read(reinterpret_cast<char*>(&a), 1);
            color col;
            col.r = r; col.g = g; col.b = b; col.a = a;
            vox->palette[i] = col;
        }
        // If the first palette entry (index 0) is meant to be transparent and not used for voxels,
        // it might be 0x00000000. We keep it in the palette vector regardless.
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_MATT(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // MATT chunk (deprecated in newer versions): material properties for one color index
        // Structure: int32 id, int32 materialType, float materialWeight, int32 propertyBits,
        // then optional floats for each property present.
        if (contentBytes < 4 * 3) { // at least id, type, weight, propertyBits = 16 bytes
            // Malformed MATT chunk, skip it entirely
            voxFile.seekg(contentBytes + childrenBytes, std::ios::cur);
            return;
        }
        uint32_t matId;
        uint32_t type;
        float weight;
        uint32_t propertyBits;
        voxFile.read(reinterpret_cast<char*>(&matId), 4);
        voxFile.read(reinterpret_cast<char*>(&type), 4);
        voxFile.read(reinterpret_cast<char*>(&weight), 4);
        voxFile.read(reinterpret_cast<char*>(&propertyBits), 4);

        vox_MATL& material = vox->materials[matId];
        // Initialize defaults for this material
        material.diffuse = 0.0f;
        material.metal = 0.0f;
        material.glass = 0.0f;
        material.emit = 0.0f;
        material.plastic = 0.0f;
        material.rough = 0.0f;
        material.spec = 0.0f;
        material.ior = 0.0f;
        material.att = 0.0f;
        material.flux = 0.0f;
        material.weight = 0.0f;
        // Set material type flags
        if (type == 0) {        // diffuse
            material.diffuse = 1.0f;
        }
        else if (type == 1) { // metal
            material.metal = 1.0f;
        }
        else if (type == 2) { // glass
            material.glass = 1.0f;
        }
        else if (type == 3) { // emissive
            material.emit = 1.0f;
        }
        material.weight = weight;

        // Property bits indicate which additional properties are present
        if (propertyBits & 0x1) { // Plastic
            float plasticVal;
            voxFile.read(reinterpret_cast<char*>(&plasticVal), 4);
            material.plastic = plasticVal;
        }
        if (propertyBits & 0x2) { // Roughness
            float roughnessVal;
            voxFile.read(reinterpret_cast<char*>(&roughnessVal), 4);
            material.rough = roughnessVal;
        }
        if (propertyBits & 0x4) { // Specular
            float specularVal;
            voxFile.read(reinterpret_cast<char*>(&specularVal), 4);
            material.spec = specularVal;
        }
        if (propertyBits & 0x8) { // IOR (index of refraction)
            float iorVal;
            voxFile.read(reinterpret_cast<char*>(&iorVal), 4);
            material.ior = iorVal;
        }
        if (propertyBits & 0x10) { // Attenuation
            float attVal;
            voxFile.read(reinterpret_cast<char*>(&attVal), 4);
            material.att = attVal;
        }
        float emissivePower = 0.0f;
        float emissiveGlow = 0.0f;
        if (propertyBits & 0x20) { // Emissive power
            voxFile.read(reinterpret_cast<char*>(&emissivePower), 4);
        }
        if (propertyBits & 0x40) { // Emissive glow
            voxFile.read(reinterpret_cast<char*>(&emissiveGlow), 4);
        }
        if (propertyBits & 0x80) { // IsTotalPower (unused here)
            float dummy;
            voxFile.read(reinterpret_cast<char*>(&dummy), 4);
        }
        // If emissive, combine power and glow into flux intensity (approximation)
        if (material.emit > 0.0f) {
            // Base flux from power (power is an exponent in older format)
            material.flux = (emissivePower != 0.0f ? std::pow(10.0f, emissivePower) : 1.0f);
            // Incorporate glow as a multiplier (glow adds extra bloom without increasing actual light)
            material.flux *= (1.0f + emissiveGlow);
        }

        // Skip any declared children bytes (none expected for MATT)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_MATL(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // MATL chunk: int32 material id, DICT of material properties (keys and values as strings)
        uint32_t matId;
        voxFile.read(reinterpret_cast<char*>(&matId), 4);
        vox_MATL& material = vox->materials[matId];
        // Set default material values
        material.diffuse = 0.0f;
        material.metal = 0.0f;
        material.glass = 0.0f;
        material.emit = 0.0f;
        material.plastic = 0.0f;
        material.rough = 0.0f;
        material.spec = 0.0f;
        material.ior = 0.0f;
        material.att = 0.0f;
        material.flux = 0.0f;
        material.weight = 0.0f;
        // By default, assume diffuse unless type says otherwise
        material.diffuse = 1.0f;

        // Read DICT of key-value material properties
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen;
            voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0');
            voxFile.read(&key[0], keyLen);
            uint32_t valLen;
            voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0');
            voxFile.read(&val[0], valLen);

            if (key == "_type") {
                // Material type: _diffuse, _metal, _glass, _emit
                if (val == "_metal") {
                    material.diffuse = 0.0f;
                    material.metal = 1.0f;
                }
                else if (val == "_glass") {
                    material.diffuse = 0.0f;
                    material.glass = 1.0f;
                }
                else if (val == "_emit") {
                    material.diffuse = 0.0f;
                    material.emit = 1.0f;
                }
                else {
                    // "_diffuse" or unknown defaults to diffuse
                    material.diffuse = 1.0f;
                }
            }
            else if (key == "_weight") {
                material.weight = std::stof(val);
            }
            else if (key == "_rough") {
                material.rough = std::stof(val);
            }
            else if (key == "_spec") {
                material.spec = std::stof(val);
            }
            else if (key == "_ior") {
                material.ior = std::stof(val);
            }
            else if (key == "_att") {
                material.att = std::stof(val);
            }
            else if (key == "_flux") {
                material.flux = std::stof(val);
            }
            else if (key == "_plastic") {
                // _plastic is treated as boolean (0 or 1)
                material.plastic = std::stof(val);
            }
            else {
                // Ignore any other material property keys
            }
        }
        // Skip children chunks if any (none expected for MATL)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_nTRN(std::shared_ptr<vox_file> vox, std::ifstream& voxFile, uint32_t contentBytes, uint32_t childrenBytes)
    {
        // nTRN (Transform Node) chunk structure:
        // int32 nodeId
        // DICT nodeAttributes
        // int32 childNodeId
        // int32 reserved (-1)
        // int32 layerId
        // int32 numFrames
        // then numFrames * { DICT frameAttributes }
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nTRN& transform = vox->transforms[nodeId];
        transform.nodeID = nodeId;
        // Read node attributes dictionary
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen;
            voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0');
            voxFile.read(&key[0], keyLen);
            uint32_t valLen;
            voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0');
            voxFile.read(&val[0], valLen);
            // Store known attributes (e.g., _name or _hidden) if needed:
            transform.attributes[key] = val;
        }
        // Read child node, reserved, layer, and number of frames
        voxFile.read(reinterpret_cast<char*>(&transform.childNodeID), 4);
        int32_t reserved;
        voxFile.read(reinterpret_cast<char*>(&reserved), 4);
        voxFile.read(reinterpret_cast<char*>(&transform.layerID), 4);
        uint32_t numFrames;
        voxFile.read(reinterpret_cast<char*>(&numFrames), 4);
        transform.framesCount = numFrames;
        transform.frameAttrib.resize(numFrames);

        for (uint32_t f = 0; f < numFrames; ++f) {
            vox_frame_attrib& frameAttrib = transform.frameAttrib[f];
            frameAttrib.frameIndex = f;
            frameAttrib.translation = { 0, 0, 0 };
            frameAttrib.rotation = vox_mat3::identity;  // identity matrix initially
            // Read frame attributes dictionary
            uint32_t frameKvCount;
            voxFile.read(reinterpret_cast<char*>(&frameKvCount), 4);
            for (uint32_t i = 0; i < frameKvCount; ++i) {
                uint32_t keyLen;
                voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
                std::string key(keyLen, '\0');
                voxFile.read(&key[0], keyLen);
                uint32_t valLen;
                voxFile.read(reinterpret_cast<char*>(&valLen), 4);
                if (key == "_r") {
                    // Rotation (stored as string of an integer)
                    std::string rotStr(valLen, '\0');
                    voxFile.read(&rotStr[0], valLen);
                    uint32_t rotBits = std::atoi(rotStr.c_str());
                    // Decode rotation from bits (MagicaVoxel uses a packed 3-bit representation)
                    // Determine which basis vectors correspond to the rows
                    uint32_t row0Index = rotBits & 0x3;
                    uint32_t row1Index = (rotBits >> 2) & 0x3;
                    // The third row index can be deduced (0,1,2 are axes)
                    static const uint32_t kRow2IndexLUT[8] = {
                        UINT32_MAX, 2, 1, UINT32_MAX,
                        0, UINT32_MAX, UINT32_MAX, UINT32_MAX
                    };
                    uint32_t mask = (1u << row0Index) | (1u << row1Index);
                    uint32_t row2Index = 0;
                    // find index not in mask (0x7=0b111 for axes 0,1,2)
                    for (uint32_t bit = 0; bit < 3; ++bit) {
                        if (!(mask & (1u << bit))) { row2Index = bit; break; }
                    }
                    // Sign bits
                    bool row0Neg = (rotBits >> 4) & 1;
                    bool row1Neg = (rotBits >> 5) & 1;
                    bool row2Neg = (rotBits >> 6) & 1;
                    // Basis vectors for axes
                    vox_vec3 basis[3] = {
                        {1.0f, 0.0f, 0.0f},
                        {0.0f, 1.0f, 0.0f},
                        {0.0f, 0.0f, 1.0f}
                    };
                    vox_vec3 row0 = basis[row0Index];
                    vox_vec3 row1 = basis[row1Index];
                    vox_vec3 row2 = basis[row2Index];
                    if (row0Neg) row0 = row0 * -1;
                    if (row1Neg) row1 = row1 * -1;
                    if (row2Neg) row2 = row2 * -1;
                    // MagicaVoxel rotation is given as row vectors; convert to column-major matrix
                    vox_mat3 rotMat;
                    rotMat.m00 = row0.x; rotMat.m01 = row0.y; rotMat.m02 = row0.z;
                    rotMat.m10 = row1.x; rotMat.m11 = row1.y; rotMat.m12 = row1.z;
                    rotMat.m20 = row2.x; rotMat.m21 = row2.y; rotMat.m22 = row2.z;
                    frameAttrib.rotation = rotMat;
                }
                else if (key == "_t") {
                    // Translation (three integers as strings separated by spaces)
                    std::string tStr(valLen, '\0');
                    voxFile.read(&tStr[0], valLen);
                    // Parse three integers from tStr
                    int tx = 0, ty = 0, tz = 0;
                    std::sscanf(tStr.c_str(), "%d %d %d", &tx, &ty, &tz);
                    frameAttrib.translation.x = tx;
                    frameAttrib.translation.y = ty;
                    frameAttrib.translation.z = tz;
                }
                else if (key == "_f") {
                    // Frame index (usually -1 or 0, not used in current versions as numFrames is typically 1)
                    std::string fStr(valLen, '\0');
                    voxFile.read(&fStr[0], valLen);
                    // We won't use frameAttrib.frameIndex here since it's set by loop
                }
                else {
                    // Unknown frame attribute: skip its value
                    voxFile.seekg(valLen, std::ios::cur);
                }
            }
        }
        // Skip any children chunks of nTRN (e.g., none expected, as frames are within content)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_nGRP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // nGRP (Group Node) chunk:
        // int32 nodeId
        // DICT nodeAttributes
        // int32 numChildren
        // int32 childNodeIds[numChildren]
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nGRP& group = vox->groups[nodeId];
        group.nodeID = nodeId;
        // Read group attributes dictionary
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen;
            voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0');
            voxFile.read(&key[0], keyLen);
            uint32_t valLen;
            voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0');
            voxFile.read(&val[0], valLen);
            // Store known attributes if needed (e.g., _name), otherwise ignore
            group.attributes[key] = val;
        }
        // Read children
        uint32_t numChildren;
        voxFile.read(reinterpret_cast<char*>(&numChildren), 4);
        group.childrenIDs.resize(numChildren);
        for (uint32_t i = 0; i < numChildren; ++i) {
            voxFile.read(reinterpret_cast<char*>(&group.childrenIDs[i]), 4);
        }
        // Skip any children chunks of nGRP (none expected, childrenIDs are in content)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_nSHP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // nSHP (Shape Node) chunk:
        // int32 nodeId
        // DICT nodeAttributes
        // int32 numModels (usually 1)
        // For each model: { int32 modelId, DICT modelAttributes }
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nSHP& shape = vox->shapes[nodeId];
        shape.nodeID = nodeId;
        // Read shape node attributes dictionary
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen;
            voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0');
            voxFile.read(&key[0], keyLen);
            uint32_t valLen;
            voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0');
            voxFile.read(&val[0], valLen);
            // Store known attributes if needed (e.g., _name or _hidden), else ignore
            shape.attributes[key] = val;
        }
        // Read models
        uint32_t numModels;
        voxFile.read(reinterpret_cast<char*>(&numModels), 4);
        shape.models.resize(numModels);
        for (uint32_t i = 0; i < numModels; ++i) {
            vox_nSHP_model& modelRef = shape.models[i];
            voxFile.read(reinterpret_cast<char*>(&modelRef.modelID), 4);
            // Read model attributes dictionary (reserved usage, e.g., _f frame index)
            uint32_t modelKvCount;
            voxFile.read(reinterpret_cast<char*>(&modelKvCount), 4);
            for (uint32_t j = 0; j < modelKvCount; ++j) {
                uint32_t keyLen;
                voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
                std::string key(keyLen, '\0');
                voxFile.read(&key[0], keyLen);
                uint32_t valLen;
                voxFile.read(reinterpret_cast<char*>(&valLen), 4);
                std::string val(valLen, '\0');
                voxFile.read(&val[0], valLen);
                if (key == "_f") {
                    // The _f key might specify an animation frame index for this model reference
                    modelRef.frameIndex = std::stoi(val);
                }
                else {
                    // Ignore other model attributes if any
                }
            }
        }
        // Skip any children chunks of nSHP (none expected)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_LAYR(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        // LAYR (Layer Definition) chunk:
        // int32 layerId
        // DICT layerAttributes (e.g., _name, _hidden)
        // int32 reserved (must be -1)
        uint32_t layerId;
        voxFile.read(reinterpret_cast<char*>(&layerId), 4);
        vox_layer layerInfo;
        layerInfo.layerID = layerId;
        layerInfo.name = "";
        layerInfo.hidden = false;
        // Read layer attributes dictionary
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen;
            voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0');
            voxFile.read(&key[0], keyLen);
            uint32_t valLen;
            voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0');
            voxFile.read(&val[0], valLen);
            if (key == "_name") {
                layerInfo.name = val;
            }
            else if (key == "_hidden") {
                // "_hidden" value is "0" or "1"
                layerInfo.hidden = (val == "1");
            }
            else {
                // ignore other layer attributes if any
            }
        }
        // Read and discard reserved int
        int32_t reserved;
        voxFile.read(reinterpret_cast<char*>(&reserved), 4);
        // Store layer info
        vox->layers[layerId] = layerInfo;
        // Skip any children chunks of LAYR (none expected)
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    // Default palette (256 colors) as per MagicaVoxel specification
    const std::vector<unsigned int> VoxParser::default_palette = {
        0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff,
        0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff,
        0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6699ff, 0xff3399ff,
        0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff,
        0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff,
        0xff0033ff, 0xffff00ff, 0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff,
        0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc,
        0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
        0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc,
        0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc,
        0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc, 0xff6633cc, 0xff3333cc,
        0xff0033cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99,
        0xff00ff99, 0xffffcc99, 0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99,
        0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999,
        0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
        0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399,
        0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099,
        0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff66ff66, 0xff33ff66,
        0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66,
        0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966,
        0xff009966, 0xffff6666, 0xffcc6666, 0xff996666, 0xff666666, 0xff336666,
        0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366,
        0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
        0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33,
        0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33,
        0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff669933, 0xff339933,
        0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633,
        0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333,
        0xff003333, 0xffff0033, 0xffcc0033, 0xff990033, 0xff660033, 0xff330033,
        0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00,
        0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
        0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900,
        0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600,
        0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa,
        0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100,
        0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,
        0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff, 0xff6633ff, 0xff3333ff,
        0xff0033ff, 0xffff00ff, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
        0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc, 0xff66ff99, 0xff33ff99,
        0xff00ff99, 0xffffcc99, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
        0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66, 0xff669966, 0xff339966,
        0xff009966, 0xffff6666, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
        0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933, 0xff663333, 0xff333333,
        0xff003333, 0xffff0033, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
        0xff006600, 0xffff3300, 0xffcc3300, 0xff993300, 0xff000088, 0xff000077,
        0xff000055, 0xff000044, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
        0xff555555, 0xff444444, 0xff222222, 0xff111111
    };
}