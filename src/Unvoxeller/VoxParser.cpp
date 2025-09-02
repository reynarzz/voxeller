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
#include <unordered_map>

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

    vox_header VoxParser::read_vox_metadata(const void* /*bytes*/)
    {
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
        vox->layers.clear();

        vox->palette.resize(default_palette.size());
        for (size_t i = 0; i < default_palette.size(); ++i)
        {
            uint32_t c = default_palette[i];
            vox->palette[i].r = uint8_t((c >> 0) & 0xFF);
            vox->palette[i].g = uint8_t((c >> 8) & 0xFF);
            vox->palette[i].b = uint8_t((c >> 16) & 0xFF);
            vox->palette[i].a = uint8_t((c >> 24) & 0xFF);
        }

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
            return nullptr;
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
        uint32_t mainContentBytes = 0;
        uint32_t mainChildrenBytes = 0;
        voxFile.read(reinterpret_cast<char*>(&mainContentBytes), 4);
        voxFile.read(reinterpret_cast<char*>(&mainChildrenBytes), 4);

        // Loop through all chunks inside MAIN
        modelIndex = 0;
        while (true) {
            char chunkId[4];
            if (!voxFile.read(chunkId, 4)) {
                break; // EOF
            }
            uint32_t chunkContentBytes = 0;
            uint32_t chunkChildrenBytes = 0;
            voxFile.read(reinterpret_cast<char*>(&chunkContentBytes), 4);
            voxFile.read(reinterpret_cast<char*>(&chunkChildrenBytes), 4);
            std::string chunkStr(chunkId, 4);

            if (chunkStr == "PACK") {
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
                voxFile.seekg(chunkContentBytes + chunkChildrenBytes, std::ios::cur);
            }
        }

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

        // -------------------------------
        // POST-PASS: build parent links
        // -------------------------------
        // Map: childNodeID (of nTRN) -> nTRN id
        std::unordered_map<int, int> trnByChild;
        trnByChild.reserve(vox->transforms.size());
        for (const auto& kv : vox->transforms) {
            trnByChild[kv.second.childNodeID] = kv.first;
        }

        // Map: nodeId -> its immediate parent group node (if any)
        std::unordered_map<int, int> parentGroupOfNode;
        for (const auto& gkv : vox->groups) {
            const int grpId = gkv.first;
            for (int childId : gkv.second.childrenIDs) {
                parentGroupOfNode[childId] = grpId;
            }
        }

        // For each transform node, walk up via groups to find the parent transform
        for (auto& kv : vox->transforms) {
            vox_nTRN& trn = kv.second;
            trn.parentNodeID = -1;  // default: root

            int cur = trn.nodeID;
            // climb: node -> parent group -> (maybe parent group of that group) ... until a TRN owns that group as child
            while (true) {
                auto pg = parentGroupOfNode.find(cur);
                if (pg == parentGroupOfNode.end()) {
                    break; // reached root (no parent group)
                }
                int grpId = pg->second;

                auto pt = trnByChild.find(grpId);
                if (pt != trnByChild.end()) {
                    trn.parentNodeID = pt->second; // found the parent transform id
                    break;
                }

                // No TRN owns this group directly; continue climbing as if the group were "cur"
                cur = grpId;
            }
        }
        // -------------------------------

        vox->isValid = true;
        return vox;
    }

    void VoxParser::parse_PACK(std::shared_ptr<vox_file> /*vox*/, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        if (contentBytes >= 4) {
            uint32_t numModels = 0;
            voxFile.read(reinterpret_cast<char*>(&numModels), 4);
        }
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_SIZE(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        if (contentBytes != 12) {
            std::cerr << "Unexpected SIZE chunk length: " << contentBytes << '\n';
        }
        vox_size size;
        voxFile.read(reinterpret_cast<char*>(&size.x), 4);
        voxFile.read(reinterpret_cast<char*>(&size.y), 4);
        voxFile.read(reinterpret_cast<char*>(&size.z), 4);
        vox->sizes.push_back(size);
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_XYZI(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes)
    {
        uint32_t numVoxels = 0;
        voxFile.read(reinterpret_cast<char*>(&numVoxels), 4);
        vox_model model;
        model.voxels.resize(numVoxels);

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

        model.boundingBox.minX = std::numeric_limits<float>::infinity();
        model.boundingBox.minY = std::numeric_limits<float>::infinity();
        model.boundingBox.minZ = std::numeric_limits<float>::infinity();
        model.boundingBox.maxX = -std::numeric_limits<float>::infinity();
        model.boundingBox.maxY = -std::numeric_limits<float>::infinity();
        model.boundingBox.maxZ = -std::numeric_limits<float>::infinity();

        for (uint32_t i = 0; i < numVoxels; ++i) {
            vox_voxel vv;
            uint8_t xi, yi, zi, ci;
            voxFile.read(reinterpret_cast<char*>(&xi), 1);
            voxFile.read(reinterpret_cast<char*>(&yi), 1);
            voxFile.read(reinterpret_cast<char*>(&zi), 1);
            voxFile.read(reinterpret_cast<char*>(&ci), 1);
            vv.x = xi; vv.y = yi; vv.z = zi; vv.colorIndex = ci;
            model.voxels[i] = vv;

            model.voxel_3dGrid[zi][yi][xi] = static_cast<int>(i);

            model.boundingBox.minX = std::min(model.boundingBox.minX, static_cast<float>(xi));
            model.boundingBox.minY = std::min(model.boundingBox.minY, static_cast<float>(yi));
            model.boundingBox.minZ = std::min(model.boundingBox.minZ, static_cast<float>(zi));
            model.boundingBox.maxX = std::max(model.boundingBox.maxX, static_cast<float>(xi + 1));
            model.boundingBox.maxY = std::max(model.boundingBox.maxY, static_cast<float>(yi + 1));
            model.boundingBox.maxZ = std::max(model.boundingBox.maxZ, static_cast<float>(zi + 1));
        }

        vox->voxModels.push_back(std::move(model));

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_RGBA(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        const size_t paletteSize = 256;
        vox->palette.resize(paletteSize);
        for (size_t i = 0; i < paletteSize; ++i) {
            uint8_t r, g, b, a;
            voxFile.read(reinterpret_cast<char*>(&r), 1);
            voxFile.read(reinterpret_cast<char*>(&g), 1);
            voxFile.read(reinterpret_cast<char*>(&b), 1);
            voxFile.read(reinterpret_cast<char*>(&a), 1);
            vox->palette[i] = { r, g, b, a };
        }
        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_MATT(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t contentBytes, uint32_t childrenBytes) {
        if (contentBytes < 16) {
            voxFile.seekg(contentBytes + childrenBytes, std::ios::cur);
            return;
        }
        uint32_t matId, type, propertyBits;
        float weight;
        voxFile.read(reinterpret_cast<char*>(&matId), 4);
        voxFile.read(reinterpret_cast<char*>(&type), 4);
        voxFile.read(reinterpret_cast<char*>(&weight), 4);
        voxFile.read(reinterpret_cast<char*>(&propertyBits), 4);

        vox_MATL& material = vox->materials[matId];
        material = {}; // zero-init
        if (type == 0) material.diffuse = 1.0f;
        else if (type == 1) material.metal = 1.0f;
        else if (type == 2) material.glass = 1.0f;
        else if (type == 3) material.emit = 1.0f;
        material.weight = weight;

        if (propertyBits & 0x1) { float v; voxFile.read(reinterpret_cast<char*>(&v), 4); material.plastic = v; }
        if (propertyBits & 0x2) { float v; voxFile.read(reinterpret_cast<char*>(&v), 4); material.rough = v; }
        if (propertyBits & 0x4) { float v; voxFile.read(reinterpret_cast<char*>(&v), 4); material.spec = v; }
        if (propertyBits & 0x8) { float v; voxFile.read(reinterpret_cast<char*>(&v), 4); material.ior = v; }
        if (propertyBits & 0x10) { float v; voxFile.read(reinterpret_cast<char*>(&v), 4); material.att = v; }
        float emissivePower = 0.0f, emissiveGlow = 0.0f;
        if (propertyBits & 0x20) { voxFile.read(reinterpret_cast<char*>(&emissivePower), 4); }
        if (propertyBits & 0x40) { voxFile.read(reinterpret_cast<char*>(&emissiveGlow), 4); }
        if (propertyBits & 0x80) { float dummy; voxFile.read(reinterpret_cast<char*>(&dummy), 4); }
        if (material.emit > 0.0f) {
            material.flux = (emissivePower != 0.0f ? std::pow(10.0f, emissivePower) : 1.0f) * (1.0f + emissiveGlow);
        }

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_MATL(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes) {
        uint32_t matId;
        voxFile.read(reinterpret_cast<char*>(&matId), 4);
        vox_MATL& material = vox->materials[matId];
        material = {};
        material.diffuse = 1.0f;

        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
            uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);

            if (key == "_type") {
                material.diffuse = material.metal = material.glass = material.emit = 0.0f;
                if (val == "_metal") material.metal = 1.0f;
                else if (val == "_glass") material.glass = 1.0f;
                else if (val == "_emit")  material.emit = 1.0f;
                else                      material.diffuse = 1.0f;
            }
            else if (key == "_weight") material.weight = std::stof(val);
            else if (key == "_rough")  material.rough = std::stof(val);
            else if (key == "_spec")   material.spec = std::stof(val);
            else if (key == "_ior")    material.ior = std::stof(val);
            else if (key == "_att")    material.att = std::stof(val);
            else if (key == "_flux")   material.flux = std::stof(val);
            else if (key == "_plastic")material.plastic = std::stof(val);
        }

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    inline glm::mat3 MatFromRows(const glm::vec3& r0,
        const glm::vec3& r1,
        const glm::vec3& r2)
    {
        return glm::mat3(
            glm::vec3(r0.x, r1.x, r2.x),
            glm::vec3(r0.y, r1.y, r2.y),
            glm::vec3(r0.z, r1.z, r2.z)
        );
    }

    static inline glm::mat3 DecodeVoxRotation(uint32_t rotBits) 
    {
        rotBits &= 0x7F; // keep bits 0..6
        const uint32_t i0 = rotBits & 0x3; // row0 index
        const uint32_t i1 = (rotBits >> 2) & 0x3; // row1 index
        if (i0 > 2 || i1 > 2 || i0 == i1) {
            return glm::mat3(1.0f); // invalid -> identity
        }
        uint32_t used = (1u << i0) | (1u << i1);
        uint32_t i2 = (used & 1u) ? ((used & 2u) ? 2u : 1u) : 0u;

        glm::vec3 basis[3] = { {1,0,0},{0,1,0},{0,0,1} };

        glm::vec3 r0 = basis[i0];
        glm::vec3 r1 = basis[i1];
        glm::vec3 r2 = basis[i2];

        if ((rotBits >> 4) & 1u) r0 = -r0;
        if ((rotBits >> 5) & 1u) r1 = -r1;
        if ((rotBits >> 6) & 1u) r2 = -r2;

        // Build for GLM (columns = basis vectors):
        return glm::mat3(
            glm::vec3(r0.x, r1.x, r2.x),
            glm::vec3(r0.y, r1.y, r2.y),
            glm::vec3(r0.z, r1.z, r2.z)
        );
    }

    void VoxParser::parse_nTRN(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes)
    {
        // nTRN layout:
        // int32 nodeId
        // DICT nodeAttributes
        // int32 childNodeId
        // int32 reserved (-1)
        // int32 layerId
        // int32 numFrames
        // numFrames * DICT frameAttribs
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nTRN& transform = vox->transforms[nodeId];
        transform.nodeID = nodeId;
        transform.parentNodeID = -1; // we'll link it in a post-pass

        // node DICT
        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
            uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);
            transform.attributes[key] = val;
        }

        voxFile.read(reinterpret_cast<char*>(&transform.childNodeID), 4);

        int32_t reservedMinusOne;
        voxFile.read(reinterpret_cast<char*>(&reservedMinusOne), 4); // ignore; NOT a parent id

        voxFile.read(reinterpret_cast<char*>(&transform.layerID), 4);

        uint32_t numFrames = 0;
        voxFile.read(reinterpret_cast<char*>(&numFrames), 4);
        transform.framesCount = numFrames;
        transform.frameAttrib.resize(numFrames);

        for (uint32_t f = 0; f < numFrames; ++f) {
            vox_frame_attrib& fa = transform.frameAttrib[f];
            fa.frameIndex = static_cast<s32>(f);
            fa.translation = { 0,0,0 };
            fa.rotation = glm::mat3(1.0f);

            uint32_t frameKvCount;
            voxFile.read(reinterpret_cast<char*>(&frameKvCount), 4);
            for (uint32_t i = 0; i < frameKvCount; ++i) {
                uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
                std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
                uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);

                if (key == "_r") 
                {
                    std::string rotStr(valLen, '\0'); voxFile.read(&rotStr[0], valLen);
                    uint32_t rotBits = static_cast<uint32_t>(std::atoi(rotStr.c_str()));
                    fa.rotation = DecodeVoxRotation(rotBits);
                }
                else if (key == "_t") 
                {
                    std::string tStr(valLen, '\0'); voxFile.read(&tStr[0], valLen);
                    int tx = 0, ty = 0, tz = 0; std::sscanf(tStr.c_str(), "%d %d %d", &tx, &ty, &tz);
                    fa.translation = glm::vec3(tx, ty, tz);
                }
                else {
                    // read & ignore unknown value
                    voxFile.seekg(valLen, std::ios::cur);
                }
            }
        }

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_nGRP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes) {
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nGRP& group = vox->groups[nodeId];
        group.nodeID = nodeId;

        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
            uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);
            group.attributes[key] = val;
        }

        uint32_t numChildren;
        voxFile.read(reinterpret_cast<char*>(&numChildren), 4);
        group.childrenIDs.resize(numChildren);
        for (uint32_t i = 0; i < numChildren; ++i) {
            voxFile.read(reinterpret_cast<char*>(&group.childrenIDs[i]), 4);
        }

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_nSHP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes) {
        uint32_t nodeId;
        voxFile.read(reinterpret_cast<char*>(&nodeId), 4);
        vox_nSHP& shape = vox->shapes[nodeId];
        shape.nodeID = nodeId;

        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
            uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);
            shape.attributes[key] = val;
        }

        uint32_t numModels;
        voxFile.read(reinterpret_cast<char*>(&numModels), 4);
        shape.models.resize(numModels);
        for (uint32_t i = 0; i < numModels; ++i) {
            vox_nSHP_model& modelRef = shape.models[i];
            voxFile.read(reinterpret_cast<char*>(&modelRef.modelID), 4);

            uint32_t modelKvCount;
            voxFile.read(reinterpret_cast<char*>(&modelKvCount), 4);
            for (uint32_t j = 0; j < modelKvCount; ++j) {
                uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
                std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
                uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
                std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);
                if (key == "_f") {
                    modelRef.frameIndex = std::stoi(val);
                }
            }
        }

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

    void VoxParser::parse_LAYR(std::shared_ptr<vox_file> vox, std::ifstream& voxFile,
        uint32_t /*contentBytes*/, uint32_t childrenBytes) {
        uint32_t layerId;
        voxFile.read(reinterpret_cast<char*>(&layerId), 4);
        vox_layer layerInfo;
        layerInfo.layerID = layerId;
        layerInfo.name = "";
        layerInfo.hidden = false;

        uint32_t kvCount;
        voxFile.read(reinterpret_cast<char*>(&kvCount), 4);
        for (uint32_t i = 0; i < kvCount; ++i) {
            uint32_t keyLen; voxFile.read(reinterpret_cast<char*>(&keyLen), 4);
            std::string key(keyLen, '\0'); voxFile.read(&key[0], keyLen);
            uint32_t valLen; voxFile.read(reinterpret_cast<char*>(&valLen), 4);
            std::string val(valLen, '\0'); voxFile.read(&val[0], valLen);
            if (key == "_name")   layerInfo.name = val;
            else if (key == "_hidden") layerInfo.hidden = (val == "1");
        }

        int32_t reserved;
        voxFile.read(reinterpret_cast<char*>(&reserved), 4);
        vox->layers[layerId] = layerInfo;

        if (childrenBytes > 0) {
            voxFile.seekg(childrenBytes, std::ios::cur);
        }
    }

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
        0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc, 0xff66ff99, 0xff33ff99,
        0xff00ff99, 0xffffcc99, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
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
