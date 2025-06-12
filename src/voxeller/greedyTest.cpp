#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <voxeller/GreedyTest.h>

// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <voxeller/VoxParser.h>


// Include stb_image_write for saving texture atlas as PNG
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// Assume the Voxeller namespace and structures from the provided data structure are available:
using namespace Voxeller;

// A simple structure for 3D vertex with position, normal, UV
struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

// Key for hashing vertex position+UV to merge vertices (for smooth shading)
struct VertKey {
    int px_i, py_i, pz_i;    // quantized position (as integers to avoid float issues)
    int u_i, v_i;            // quantized UV (as integers in texture pixel space)
    // Note: We quantize UV by multiplying by texture width/height when comparing keys.
    bool operator==(const VertKey& other) const {
        return px_i == other.px_i && py_i == other.py_i && pz_i == other.pz_i 
            && u_i == other.u_i && v_i == other.v_i;
    }
};

// Hash for VertKey
struct VertKeyHash {
    size_t operator()(const VertKey& key) const noexcept {
        // combine the components into one hash (using 64-bit to avoid overflow)
        uint64_t hash = ((uint64_t)(key.px_i * 73856093) ^ (uint64_t)(key.py_i * 19349663) ^ (uint64_t)(key.pz_i * 83492791));
        hash ^= ((uint64_t)(key.u_i * 4256249) ^ (uint64_t)(key.v_i * 253 ^ (hash >> 32)));
        return (size_t)hash;
    }
};

// Structure to hold a face (rectangle) that needs to be packed into the atlas
struct FaceRect {
    int w, h;        // dimensions (including border will be added around)
    int atlasX, atlasY; // top-left position in atlas (including border) after packing
    uint8_t colorIndex; // palette index (for color)
    // face orientation and extents for UV mapping
    char orientation; // one of: 'X','x','Y','y','Z','z' for +X, -X, +Y, -Y, +Z, -Z
    // Face extents in voxel coordinates (the *inclusive* start and end boundaries of the face in world coordinates)
    int uMin, uMax;  // min and max along face's U-axis (in world coords, face boundary)
    int vMin, vMax;  // min and max along face's V-axis
    int constantCoord; // the coordinate of the face plane (e.g., x or y or z value for the face)
};

// Performs greedy meshing on a single vox_model to extract faces and merge contiguous ones.
// Returns a list of FaceRect representing all merged faces (with orientation and extents), and sets usedColors (unique palette indices used).
static std::vector<FaceRect> GreedyMeshModel(const vox_model& model, const vox_size& size, std::unordered_set<uint8_t>& usedColors) {
    int sizex = size.x;
    int sizey = size.y;
    int sizez = size.z;
    assert(sizex > 0 && sizey > 0 && sizez > 0);
    std::vector<FaceRect> faces;
    faces.reserve(1000); // reserve some space (will grow if needed)

    // Lambda to check if a voxel exists at given coordinates
    auto isVoxelFilled = [&](int x, int y, int z)->bool {
        if(x < 0 || x >= sizex || y < 0 || y >= sizey || z < 0 || z >= sizez) return false;
        int cell = model.voxel_3dGrid[z][y][x];
        // We consider a voxel filled if voxel_3dGrid gives a valid index (and possibly color index not 0 if 0 means empty).
        // We'll treat any non-negative cell as filled. If color index 0 is used as empty by convention, check alpha as well if needed.
        if(cell < 0) return false;
        // cell might be color index or pointer to palette index. In parsing, likely voxel_3dGrid stores color index directly.
        // We'll trust that cell >= 0 implies a voxel. We might refine by checking palette alpha to skip transparent voxels:
        if(cell < model.voxels.size()) {
            uint8_t colorIndex = model.voxels[cell].colorIndex;
            // If colorIndex is 0 and palette[0].a == 0, treat as empty.
            // In MagicaVoxel, colorIndex 0 is often unused or transparent.
            if(colorIndex == 0) {
                // Check palette alpha if available
                // (MagicaVoxel's default palette index 0 is transparent by default).
                // If the palette vector has an entry at 0, use it.
                if(!model.voxels.empty()) {
                    // The palette index might be global in vox_file; but model.voxels colorIndex likely 1-255, with 0 possibly meaning no voxel.
                    // We'll assume 0 means empty (so skip).
                    return false;
                }
            }
        }
        return true;
    };

    // Lambda to get the color index of a voxel at given coordinates (assuming it's filled)
    auto getColorIndex = [&](int x, int y, int z)->uint8_t {
        // The voxel_3dGrid likely stores an index to voxels vector or color index directly.
        int cell = model.voxel_3dGrid[z][y][x];
        if(cell < 0) return 0;
        uint8_t colorIdx;
        if(cell < model.voxels.size()) {
            // If voxel_3dGrid stores index into voxels vector:
            colorIdx = model.voxels[cell].colorIndex;
        } else {
            // If voxel_3dGrid directly stores color index:
            colorIdx = (uint8_t)cell;
        }
        return colorIdx;
    };

    // Greedy merge on each face orientation
    // We'll have 6 cases: +X, -X, +Y, -Y, +Z, -Z.
    // Use different loops and 2D mask for each orientation.

    // Masks and visited arrays for merging (allocated to max size needed)
    std::vector<bool> mask;
    std::vector<bool> visited;
    mask.reserve(sizey * sizez);

    // +X faces (faces normal pointing +X)
    for(int x = 0; x < sizex; ++x) {
        // create mask of size (sizey x sizez)
        mask.assign(sizey * sizez, false);
        // also store color index in a parallel array
        std::vector<uint8_t> maskColor(sizey * sizez);
        // mark face presence
        bool facePresent = false;
        for(int z = 0; z < sizez; ++z) {
            for(int y = 0; y < sizey; ++y) {
                // if voxel at (x,y,z) and either it's the last column or the voxel to +X is empty -> face
                if(isVoxelFilled(x, y, z) && (x == sizex - 1 || !isVoxelFilled(x + 1, y, z))) {
                    facePresent = true;
                    int idx = y * sizez + z;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        // visited mask for merging
        visited.assign(sizey * sizez, false);
        // greedy merge rectangles in this mask
        for(int z = 0; z < sizez; ++z) {
            for(int y = 0; y < sizey; ++y) {
                int idx = y * sizez + z;
                if(mask[idx] && !visited[idx]) {
                    // new rectangle found at (y,z)
                    uint8_t color = maskColor[idx];
                    // find width in z-direction
                    int w = 1;
                    while(z + w < sizez && mask[y * sizez + (z+w)] && !visited[y * sizez + (z+w)] 
                          && maskColor[y * sizez + (z+w)] == color) {
                        ++w;
                    }
                    // find height in y-direction
                    int h = 1;
                    bool expand = true;
                    while(expand && (y + h) < sizey) {
                        // check the next row of height
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (y+h) * sizez + (z+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) {
                            ++h;
                        }
                    }
                    // Mark all covered cells as visited
                    for(int dy = 0; dy < h; ++dy) {
                        for(int dz = 0; dz < w; ++dz) {
                            visited[(y+dy) * sizez + (z+dz)] = true;
                        }
                    }
                    // Construct face rectangle data:
                    FaceRect face;
                    face.orientation = 'X'; // +X
                    face.colorIndex = color;
                    // Face plane is at x+1 (outer surface at the boundary of voxel(s))
                    face.constantCoord = x + 1;
                    // The face spans [y..y+h-1] and [z..z+w-1] in voxel indices.
                    // Convert to world boundary coords:
                    face.uMin = z;
                    face.uMax = z + w;    // one past last voxel index along Z
                    face.vMin = y;
                    face.vMax = y + h;    // one past last voxel index along Y
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    // -X faces (faces normal pointing -X)
    for(int x = 0; x < sizex; ++x) {
        mask.assign(sizey * sizez, false);
        std::vector<uint8_t> maskColor(sizey * sizez);
        bool facePresent = false;
        for(int z = 0; z < sizez; ++z) {
            for(int y = 0; y < sizey; ++y) {
                if(isVoxelFilled(x, y, z) && (x == 0 || !isVoxelFilled(x - 1, y, z))) {
                    facePresent = true;
                    int idx = y * sizez + z;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        visited.assign(sizey * sizez, false);
        for(int z = 0; z < sizez; ++z) {
            for(int y = 0; y < sizey; ++y) {
                int idx = y * sizez + z;
                if(mask[idx] && !visited[idx]) {
                    uint8_t color = maskColor[idx];
                    // width in z-direction
                    int w = 1;
                    while(z + w < sizez && mask[y * sizez + (z+w)] && !visited[y * sizez + (z+w)] 
                          && maskColor[y * sizez + (z+w)] == color) {
                        ++w;
                    }
                    // height in y-direction
                    int h = 1;
                    bool expand = true;
                    while(expand && (y + h) < sizey) {
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (y+h) * sizez + (z+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) ++h;
                    }
                    for(int dy = 0; dy < h; ++dy) {
                        for(int dz = 0; dz < w; ++dz) {
                            visited[(y+dy) * sizez + (z+dz)] = true;
                        }
                    }
                    FaceRect face;
                    face.orientation = 'x'; // -X (using lowercase 'x' to denote negative)
                    face.colorIndex = color;
                    // Face plane is at x (the left/back face at the voxel's boundary)
                    face.constantCoord = x; 
                    // For -X face, spans same Y,Z region as above
                    face.uMin = z;
                    face.uMax = z + w;
                    face.vMin = y;
                    face.vMax = y + h;
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    // +Y faces
    for(int y = 0; y < sizey; ++y) {
        mask.assign(sizex * sizez, false);
        std::vector<uint8_t> maskColor(sizex * sizez);
        bool facePresent = false;
        for(int z = 0; z < sizez; ++z) {
            for(int x = 0; x < sizex; ++x) {
                if(isVoxelFilled(x, y, z) && (y == sizey - 1 || !isVoxelFilled(x, y + 1, z))) {
                    facePresent = true;
                    int idx = z * sizex + x;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        visited.assign(sizex * sizez, false);
        for(int z = 0; z < sizez; ++z) {
            for(int x = 0; x < sizex; ++x) {
                int idx = z * sizex + x;
                if(mask[idx] && !visited[idx]) {
                    uint8_t color = maskColor[idx];
                    // width in X-direction
                    int w = 1;
                    while(x + w < sizex && mask[z * sizex + (x+w)] && !visited[z * sizex + (x+w)]
                          && maskColor[z * sizex + (x+w)] == color) {
                        ++w;
                    }
                    // height in Z-direction
                    int h = 1;
                    bool expand = true;
                    while(expand && (z + h) < sizez) {
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (z+h) * sizex + (x+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) ++h;
                    }
                    for(int dz = 0; dz < h; ++dz) {
                        for(int dx = 0; dx < w; ++dx) {
                            visited[(z+dz) * sizex + (x+dx)] = true;
                        }
                    }
                    FaceRect face;
                    face.orientation = 'Y'; // +Y
                    face.colorIndex = color;
                    face.constantCoord = y + 1; // plane at y+1 (top face)
                    face.uMin = x;
                    face.uMax = x + w;
                    face.vMin = z;
                    face.vMax = z + h;
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    // -Y faces
    for(int y = 0; y < sizey; ++y) {
        mask.assign(sizex * sizez, false);
        std::vector<uint8_t> maskColor(sizex * sizez);
        bool facePresent = false;
        for(int z = 0; z < sizez; ++z) {
            for(int x = 0; x < sizex; ++x) {
                if(isVoxelFilled(x, y, z) && (y == 0 || !isVoxelFilled(x, y - 1, z))) {
                    facePresent = true;
                    int idx = z * sizex + x;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        visited.assign(sizex * sizez, false);
        for(int z = 0; z < sizez; ++z) {
            for(int x = 0; x < sizex; ++x) {
                int idx = z * sizex + x;
                if(mask[idx] && !visited[idx]) {
                    uint8_t color = maskColor[idx];
                    int w = 1;
                    while(x + w < sizex && mask[z * sizex + (x+w)] && !visited[z * sizex + (x+w)]
                          && maskColor[z * sizex + (x+w)] == color) {
                        ++w;
                    }
                    int h = 1;
                    bool expand = true;
                    while(expand && (z + h) < sizez) {
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (z+h) * sizex + (x+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) ++h;
                    }
                    for(int dz = 0; dz < h; ++dz) {
                        for(int dx = 0; dx < w; ++dx) {
                            visited[(z+dz) * sizex + (x+dx)] = true;
                        }
                    }
                    FaceRect face;
                    face.orientation = 'y'; // -Y
                    face.colorIndex = color;
                    face.constantCoord = y; // plane at y (bottom face)
                    face.uMin = x;
                    face.uMax = x + w;
                    face.vMin = z;
                    face.vMax = z + h;
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    // +Z faces
    for(int z = 0; z < sizez; ++z) {
        mask.assign(sizex * sizey, false);
        std::vector<uint8_t> maskColor(sizex * sizey);
        bool facePresent = false;
        for(int y = 0; y < sizey; ++y) {
            for(int x = 0; x < sizex; ++x) {
                if(isVoxelFilled(x, y, z) && (z == sizez - 1 || !isVoxelFilled(x, y, z + 1))) {
                    facePresent = true;
                    int idx = y * sizex + x;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        visited.assign(sizex * sizey, false);
        for(int y = 0; y < sizey; ++y) {
            for(int x = 0; x < sizex; ++x) {
                int idx = y * sizex + x;
                if(mask[idx] && !visited[idx]) {
                    uint8_t color = maskColor[idx];
                    int w = 1;
                    while(x + w < sizex && mask[y * sizex + (x+w)] && !visited[y * sizex + (x+w)]
                          && maskColor[y * sizex + (x+w)] == color) {
                        ++w;
                    }
                    int h = 1;
                    bool expand = true;
                    while(expand && (y + h) < sizey) {
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (y+h) * sizex + (x+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) ++h;
                    }
                    for(int dy = 0; dy < h; ++dy) {
                        for(int dx = 0; dx < w; ++dx) {
                            visited[(y+dy) * sizex + (x+dx)] = true;
                        }
                    }
                    FaceRect face;
                    face.orientation = 'Z'; // +Z
                    face.colorIndex = color;
                    face.constantCoord = z + 1;
                    face.uMin = x;
                    face.uMax = x + w;
                    face.vMin = y;
                    face.vMax = y + h;
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    // -Z faces
    for(int z = 0; z < sizez; ++z) {
        mask.assign(sizex * sizey, false);
        std::vector<uint8_t> maskColor(sizex * sizey);
        bool facePresent = false;
        for(int y = 0; y < sizey; ++y) {
            for(int x = 0; x < sizex; ++x) {
                if(isVoxelFilled(x, y, z) && (z == 0 || !isVoxelFilled(x, y, z - 1))) {
                    facePresent = true;
                    int idx = y * sizex + x;
                    mask[idx] = true;
                    uint8_t colorIdx = getColorIndex(x, y, z);
                    maskColor[idx] = colorIdx;
                    usedColors.insert(colorIdx);
                }
            }
        }
        if(!facePresent) continue;
        visited.assign(sizex * sizey, false);
        for(int y = 0; y < sizey; ++y) {
            for(int x = 0; x < sizex; ++x) {
                int idx = y * sizex + x;
                if(mask[idx] && !visited[idx]) {
                    uint8_t color = maskColor[idx];
                    int w = 1;
                    while(x + w < sizex && mask[y * sizex + (x+w)] && !visited[y * sizex + (x+w)]
                          && maskColor[y * sizex + (x+w)] == color) {
                        ++w;
                    }
                    int h = 1;
                    bool expand = true;
                    while(expand && (y + h) < sizey) {
                        for(int k = 0; k < w; ++k) {
                            int idx2 = (y+h) * sizex + (x+k);
                            if(!(mask[idx2] && !visited[idx2] && maskColor[idx2] == color)) {
                                expand = false;
                                break;
                            }
                        }
                        if(expand) ++h;
                    }
                    for(int dy = 0; dy < h; ++dy) {
                        for(int dx = 0; dx < w; ++dx) {
                            visited[(y+dy) * sizex + (x+dx)] = true;
                        }
                    }
                    FaceRect face;
                    face.orientation = 'z'; // -Z
                    face.colorIndex = color;
                    face.constantCoord = z;
                    face.uMin = x;
                    face.uMax = x + w;
                    face.vMin = y;
                    face.vMax = y + h;
                    face.w = w;
                    face.h = h;
                    faces.push_back(face);
                }
            }
        }
    }

    return faces;
}

// A simple shelf-bin packer for placing rectangles (with added border) into a square atlas of given dimension.
// Returns true and updates FaceRect atlas positions if successful, or false if not fitting.
static bool PackFacesIntoAtlas(int atlasSize, std::vector<FaceRect>& rects) {
    // Sort rectangles by height (descending) for better packing (larger first).
    std::sort(rects.begin(), rects.end(), [](const FaceRect& a, const FaceRect& b){
        // compare (h+2) including border
        int ah = a.h + 2;
        int bh = b.h + 2;
        if(ah == bh) {
            // if heights equal, maybe sort by width as well
            return (a.w + 2) > (b.w + 2);
        }
        return ah > bh;
    });
    int currentX = 0;
    int currentY = 0;
    int currentRowHeight = 0;
    int usedHeight = 0;
    for(auto& face : rects) {
        int rw = face.w + 2; // rect width with border
        int rh = face.h + 2; // rect height with border
        if(rw > atlasSize || rh > atlasSize) {
            return false; // one rect too big to ever fit
        }
        if(currentX + rw > atlasSize) {
            // start new row
            currentY += currentRowHeight;
            currentX = 0;
            currentRowHeight = 0;
        }
        if(currentY + rh > atlasSize) {
            return false; // height overflow
        }
        // place this rect
        face.atlasX = currentX;
        face.atlasY = currentY;
        // update row
        currentX += rw;
        if(rh > currentRowHeight) {
            currentRowHeight = rh;
        }
        usedHeight = std::max(usedHeight, currentY + currentRowHeight);
        if(usedHeight > atlasSize) {
            return false;
        }
    }
    return true;
}

// Build the actual geometry (vertices and indices) for a mesh from the FaceRect list and a given texture atlas configuration.
static void BuildMeshFromFaces(const std::vector<FaceRect>& faces, int texWidth, int texHeight, bool flatShading,
                               const std::vector<color>& palette, aiMesh* mesh) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(faces.size() * 4);
    indices.reserve(faces.size() * 6);
    // If smooth shading, use map to unify vertices
    std::unordered_map<VertKey, unsigned int, VertKeyHash> vertMap;
    vertMap.reserve(faces.size() * 4);

    // Lambda to add a vertex (with optional reuse if smooth)
    auto addVertex = [&](float vx, float vy, float vz, float nx, float ny, float nz, float u, float v) -> unsigned int {
        if(!flatShading) {
            // Use quantized key for position and UV to merge vertices
            VertKey key;
            // Quantize position by multiplying by 100 (positions are small integers mostly, avoid float issues)
            key.px_i = (int)std::round(vx * 100.0f);
            key.py_i = (int)std::round(vy * 100.0f);
            key.pz_i = (int)std::round(vz * 100.0f);
            // Quantize UV by texture dimensions (map to pixel coordinates)
            key.u_i = (int)std::round(u * texWidth * 100.0f);   // multiply by 100 to preserve sub-pixel if any
            key.v_i = (int)std::round(v * texHeight * 100.0f);
            auto it = vertMap.find(key);
            if(it != vertMap.end()) {
                // Vertex exists, accumulate normal
                unsigned int idx = it->second;
                vertices[idx].nx += nx;
                vertices[idx].ny += ny;
                vertices[idx].nz += nz;
                return idx;
            } else {
                // Create new vertex
                Vertex vert;
                vert.px = vx; vert.py = vy; vert.pz = vz;
                vert.nx = nx; vert.ny = ny; vert.nz = nz;
                vert.u = u; vert.v = v;
                unsigned int newIndex = vertices.size();
                vertices.push_back(vert);
                vertMap[key] = newIndex;
                return newIndex;
            }
        } else {
            // Flat shading: always create separate vertex
            Vertex vert;
            vert.px = vx; vert.py = vy; vert.pz = vz;
            vert.nx = nx; vert.ny = ny; vert.nz = nz;
            vert.u = u; vert.v = v;
            unsigned int idx = vertices.size();
            vertices.push_back(vert);
            return idx;
        }
    };

    // Process each face rect to generate two triangles
    for(const FaceRect& face : faces) {
        // Determine face normal and orientation axes
        float nx=0, ny=0, nz=0;
        // We'll map face coordinates to texture using the atlas placement.
        // Pre-calculate UV bounds (in [0,1] texture coordinate space) for this face's interior (excluding border).
        float u0 = (face.atlasX + 1) / (float)texWidth;
        float u1 = (face.atlasX + 1 + face.w - 1) / (float)texWidth; // mapping last interior pixel center
        float v0 = (face.atlasY + 1) / (float)texHeight;
        float v1 = (face.atlasY + 1 + face.h - 1) / (float)texHeight;
        // Actually, adjusting by -0.5 for center might be ideal, but since all pixels are same color, it's fine.

        // Coordinates of face corners in world:
        // We have face.uMin, face.uMax, face.vMin, face.vMax which are the face boundaries.
        // If face.orientation is +X ('X') or -X ('x'):
        //   U axis corresponds to Z, V axis to Y.
        //   For +X: normal = (1,0,0), U increases with Z, V increases with Y.
        //   For -X: normal = (-1,0,0), U increases with -Z, V increases with Y.
        if(face.orientation == 'X' || face.orientation == 'x') {
            bool pos = (face.orientation == 'X');
            nx = pos ? 1.0f : -1.0f;
            ny = 0; nz = 0;
            // Face plane X coordinate:
            float fx = (float)face.constantCoord;
            // Compute the world min/max of Z and Y boundaries
            float zmin = (float)face.uMin;
            float zmax = (float)face.uMax;
            float ymin = (float)face.vMin;
            float ymax = (float)face.vMax;
            // For -X face, we invert U axis (so we will swap zmin/zmax in mapping)
            if(!pos) std::swap(zmin, zmax);
            // Define the four corner vertices (v0 bottom-left, v1 top-left, v2 top-right, v3 bottom-right in UV space)
            // Bottom-left (ymin,zmin):
            unsigned int i0 = addVertex(fx, ymin, zmin, nx, ny, nz, u0, v0);
            // Top-left (ymax, zmin):
            unsigned int i1 = addVertex(fx, ymax, zmin, nx, ny, nz, u0, v1);
            // Top-right (ymax, zmax):
            unsigned int i2 = addVertex(fx, ymax, zmax, nx, ny, nz, u1, v1);
            // Bottom-right (ymin, zmax):
            unsigned int i3 = addVertex(fx, ymin, zmax, nx, ny, nz, u1, v0);
            // Two triangles (i0, i1, i2) and (i0, i2, i3)
            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
        // +Y or -Y:
        else if(face.orientation == 'Y' || face.orientation == 'y') {
            bool pos = (face.orientation == 'Y');
            nx = 0; ny = pos ? 1.0f : -1.0f; nz = 0;
            float fy = (float)face.constantCoord;
            float xmin = (float)face.uMin;
            float xmax = (float)face.uMax;
            float zmin = (float)face.vMin;
            float zmax = (float)face.vMax;
            // For -Y, V axis is -Z (so swap zmin,zmax in mapping)
            if(!pos) std::swap(zmin, zmax);
            // U axis for Y faces is X normally (not flipped).
            // Define corners:
            // Bottom-left (for top view, bottom is -Z direction? But we'll follow our mapping: 
            // For +Y (top face), U=X (increasing east), V=Z (increasing south).
            // v0 bottom-left = (xmin,zmin)
            unsigned int i0 = addVertex(xmin, fy, zmin, nx, ny, nz, u0, v0);
            unsigned int i1 = addVertex(xmin, fy, zmax, nx, ny, nz, u0, v1);
            unsigned int i2 = addVertex(xmax, fy, zmax, nx, ny, nz, u1, v1);
            unsigned int i3 = addVertex(xmax, fy, zmin, nx, ny, nz, u1, v0);
            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
        // +Z or -Z:
        else if(face.orientation == 'Z' || face.orientation == 'z') {
            bool pos = (face.orientation == 'Z');
            nx = 0; ny = 0; nz = pos ? 1.0f : -1.0f;
            float fz = (float)face.constantCoord;
            float xmin = (float)face.uMin;
            float xmax = (float)face.uMax;
            float ymin = (float)face.vMin;
            float ymax = (float)face.vMax;
            // For -Z, U = -X (swap xmin,xmax)
            if(!pos) std::swap(xmin, xmax);
            // U axis = X, V axis = Y for Z faces.
            unsigned int i0 = addVertex(xmin, ymin, fz, nx, ny, nz, u0, v0);
            unsigned int i1 = addVertex(xmin, ymax, fz, nx, ny, nz, u0, v1);
            unsigned int i2 = addVertex(xmax, ymax, fz, nx, ny, nz, u1, v1);
            unsigned int i3 = addVertex(xmax, ymin, fz, nx, ny, nz, u1, v0);
            indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
            indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
        }
    }

    // If smooth shading, normalize all accumulated normals
    if(!flatShading) {
        for(auto& v : vertices) {
            float length = std::sqrt(v.nx*v.nx + v.ny*v.ny + v.nz*v.nz);
            if(length > 0.0f) {
                v.nx /= length;
                v.ny /= length;
                v.nz /= length;
            }
        }
    }

    // Now fill the aiMesh structure
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    mesh->mNumVertices = (unsigned int)vertices.size();
    mesh->mVertices = new aiVector3D[mesh->mNumVertices];
    mesh->mNormals = new aiVector3D[mesh->mNumVertices];
    mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
    mesh->mNumUVComponents[0] = 2;
    for(unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        mesh->mVertices[i] = aiVector3D(vertices[i].px, vertices[i].py, vertices[i].pz);
        mesh->mNormals[i] = aiVector3D(vertices[i].nx, vertices[i].ny, vertices[i].nz);
        mesh->mTextureCoords[0][i] = aiVector3D(vertices[i].u, vertices[i].v, 0.0f);
    }
    // Faces (triangles)
    mesh->mNumFaces = (unsigned int)(indices.size() / 3);
    mesh->mFaces = new aiFace[mesh->mNumFaces];
    for(unsigned int f = 0; f < mesh->mNumFaces; ++f) {
        mesh->mFaces[f].mNumIndices = 3;
        mesh->mFaces[f].mIndices = new unsigned int[3];
        mesh->mFaces[f].mIndices[0] = indices[f*3 + 0];
        mesh->mFaces[f].mIndices[1] = indices[f*3 + 1];
        mesh->mFaces[f].mIndices[2] = indices[f*3 + 2];
    }
}

// Create and save a PNG texture from the atlas data
static bool SaveAtlasImage(const std::string& filename, int width, int height, const std::vector<unsigned char>& rgbaData) {
    // Use stb_image_write to write PNG
    if(stbi_write_png(filename.c_str(), width, height, 4, rgbaData.data(), width * 4) == 0) {
        return false;
    }
    return true;
}

// Generate the texture atlas image data given the list of faces and palette colors
static void GenerateAtlasImage(int texWidth, int texHeight, const std::vector<FaceRect>& faces, const std::vector<color>& palette, std::vector<unsigned char>& outImage) {
    outImage.assign(texWidth * texHeight * 4, 0);
    // Fill background with transparent or black? We'll use 0 alpha for clarity outside faces.
    // outImage already 0-initialized, which corresponds to RGBA(0,0,0,0).
    for(const FaceRect& face : faces) {
        // Determine color RGBA for this face
        color col = {0,0,0,255};
        if(face.colorIndex < palette.size()) {
            col = palette[face.colorIndex];
        }
        // Coordinates in image including border:
        int x0 = face.atlasX;
        int y0 = face.atlasY;
        int rw = face.w + 2;
        int rh = face.h + 2;
        // Fill interior and border:
        for(int dy = 0; dy < rh; ++dy) {
            for(int dx = 0; dx < rw; ++dx) {
                int px = x0 + dx;
                int py = y0 + dy;
                // Bounds check (should always be inside)
                if(px < 0 || px >= texWidth || py < 0 || py >= texHeight) continue;
                int idx = (py * texWidth + px) * 4;
                outImage[idx]   = col.r;
                outImage[idx+1] = col.g;
                outImage[idx+2] = col.b;
                outImage[idx+3] = col.a;
            }
        }
    }
}

bool Run(const std::string& inputPath, const std::string& outputPath)
{
    
    // Parse the .vox file
    std::shared_ptr<vox_file> voxData = VoxParser::read_vox_file(inputPath.c_str());
    if(!voxData || !voxData->isValid) {
        std::cerr << "Failed to read voxel file or file is invalid.\n";
        return 1;
    }
    // Determine if we have multiple frames (multiple models or transform frames)
    size_t frameCount = voxData->voxModels.size();
    if(frameCount == 0) {
        std::cerr << "No voxel models in the file.\n";
        return 1;
    }
    bool multiFrame = frameCount > 1;
    // If not multiple models, check transforms for framesCount
    if(!multiFrame) {
        // find any transform with framesCount > 1
        for(auto& kv : voxData->transforms) {
            if(kv.second.framesCount > 1) {
                multiFrame = true;
                frameCount = kv.second.framesCount;
                break;
            }
        }
    }

    // Ask user about exporting frames
    bool exportFramesSeparately = false;
    if(frameCount > 1) {
        std::cout << "This model has " << frameCount << " frames. Export frames as separate files? (y/n): ";
        char c;
        std::cin >> c;
        exportFramesSeparately = (c == 'y' || c == 'Y');
    }
    // Ask user about atlas vs separate textures if multiple meshes will be in one file
    bool separateTexturesPerMesh = false;
    if(!exportFramesSeparately && frameCount > 1) {
        std::cout << "Use separate texture per frame/mesh instead of one atlas? (y/n): ";
        char c;
        std::cin >> c;
        separateTexturesPerMesh = (c == 'y' || c == 'Y');
    }
    // Ask about power-of-two texture enforcement
    bool forcePowerOfTwo = true;
    std::cout << "Force texture dimensions to power-of-two? (y/n): ";
    char cpot;
    std::cin >> cpot;
    forcePowerOfTwo = (cpot == 'y' || cpot == 'Y');
    // Ask about shading (flat or smooth)
    bool flatShading = true;
    std::cout << "Flat shading (y) or smooth shading (n)? ";
    char cshade;
    std::cin >> cshade;
    flatShading = (cshade == 'y' || cshade == 'Y');

    // Set up Assimp scene
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    std::vector<std::vector<unsigned char>> images; // store image data for possibly multiple textures
    std::vector<std::string> imageFilenames;
    std::vector<aiMaterial*> materials;

    // If exporting frames separately, we'll loop and export one scene per frame instead of building one scene with multiple.
    // But we can reuse this code by simply generating one scene at a time inside the loop if separate.
    // For combined output, we build scene once.

        size_t dot = outputPath.find_last_of('.');

    auto exportScene = [&](const std::string& outName) {
        Assimp::Exporter exporter;
        // Determine export format from extension
        std::string ext;
        if(dot != std::string::npos) ext = outName.substr(dot+1);
        std::string formatId;
        const aiExportFormatDesc* selectedFormat = nullptr;
        for(size_t i = 0; i < exporter.GetExportFormatCount(); ++i) {
            const aiExportFormatDesc* fmt = exporter.GetExportFormatDescription(i);
            if(fmt && fmt->fileExtension == ext) {
                selectedFormat = fmt;
                break;
            }
        }
        if(!selectedFormat) {
            std::cerr << "Unsupported export format: " << ext << "\n";
            return false;
        }
        formatId = selectedFormat->id;
        aiReturn ret = exporter.Export(scene, formatId.c_str(), outName);
        if(ret != aiReturn_SUCCESS) {
            std::cerr << "Export failed: " << exporter.GetErrorString() << "\n";
            return false;
        }
        return true;
    };

    if(exportFramesSeparately && frameCount > 1) {
        // Loop through frames, create scene for each
        for(size_t fi = 0; fi < frameCount; ++fi) {
            // Prepare a new minimal scene for this frame
            aiScene singleScene;
            singleScene.mRootNode = new aiNode();
            // Greedy mesh for this frame
            std::unordered_set<uint8_t> usedColors;
            // If voxModels has multiple entries, use that directly
            size_t modelIndex = fi;
            if(modelIndex >= voxData->voxModels.size()) modelIndex = voxData->voxModels.size() - 1;
            std::vector<FaceRect> faces = GreedyMeshModel(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], usedColors);
            // Determine atlas size
            int atlasDim = 16;
            // Base initial size on number of used colors or face area sum
            // We'll just try increasing POT until success
            if(forcePowerOfTwo) {
                // Start from 16 and double
                while(true) {
                    if(PackFacesIntoAtlas(atlasDim, faces)) break;
                    atlasDim *= 2;
                    if(atlasDim > 4096) { // safety break
                        std::cerr << "Could not pack texture atlas up to 4096 for frame " << fi << "\n";
                        break;
                    }
                }
            } else {
                // Start with 16 and grow by 16 steps or double as needed (non-POT allowed)
                while(true) {
                    if(PackFacesIntoAtlas(atlasDim, faces)) break;
                    atlasDim += 16;
                    if(atlasDim > 4096) {
                        std::cerr << "Could not pack texture atlas within 4096 for frame " << fi << "\n";
                        break;
                    }
                }
                // Optionally shrink to actual used size
                // We can compute used width and height from faces placement
                int usedW = 0, usedH = 0;
                for(auto& fr : faces) {
                    usedW = std::max(usedW, fr.atlasX + fr.w + 2);
                    usedH = std::max(usedH, fr.atlasY + fr.h + 2);
                }
                atlasDim = std::max(usedW, usedH);
            }
            // Create image
            std::vector<unsigned char> image;
            GenerateAtlasImage(atlasDim, atlasDim, faces, voxData->palette, image);
            // Save image file for this frame
            std::string baseName = outputPath;
            if(dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
            std::string imageName = baseName + "_frame" + std::to_string(fi) + ".png";
            SaveAtlasImage(imageName, atlasDim, atlasDim, image);
            // Create material
            singleScene.mMaterials = new aiMaterial*[1];
            singleScene.mNumMaterials = 1;
            singleScene.mMaterials[0] = new aiMaterial();
            aiString texPath(imageName);
            singleScene.mMaterials[0]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
            // Create mesh
            singleScene.mMeshes = new aiMesh*[1];
            singleScene.mNumMeshes = 1;
            singleScene.mMeshes[0] = new aiMesh();
            aiMesh* mesh = singleScene.mMeshes[0];
            BuildMeshFromFaces(faces, atlasDim, atlasDim, flatShading, voxData->palette, mesh);
            mesh->mMaterialIndex = 0;
            // Attach mesh to root node
            singleScene.mRootNode->mNumMeshes = 1;
            singleScene.mRootNode->mMeshes = new unsigned int[1];
            singleScene.mRootNode->mMeshes[0] = 0;
            // Export this scene
            std::string frameOut = outputPath;
            // Insert frame number before extension
            if(dot != std::string::npos) {
                frameOut = outputPath.substr(0, outputPath.find_last_of('.')) + "_frame" + std::to_string(fi) + outputPath.substr(outputPath.find_last_of('.'));
            } else {
                frameOut = outputPath + "_frame" + std::to_string(fi);
            }
            Assimp::Exporter exporter;
            const aiExportFormatDesc* fmtDesc = nullptr;
            for(size_t ii = 0; ii < exporter.GetExportFormatCount(); ++ii) {
                auto fdesc = exporter.GetExportFormatDescription(ii);
                if(fdesc && outputPath.size() >= strlen(fdesc->fileExtension) &&
                   outputPath.substr(outputPath.size()-strlen(fdesc->fileExtension)) == fdesc->fileExtension) {
                    fmtDesc = fdesc;
                    break;
                }
            }
            std::string fmtId = fmtDesc ? fmtDesc->id : "fbx";
            if(exporter.Export(&singleScene, fmtId.c_str(), frameOut) != aiReturn_SUCCESS) {
                std::cerr << "Failed to export frame " << fi << ": " << exporter.GetErrorString() << "\n";
            } else {
                std::cout << "Exported " << frameOut << "\n";
            }
            // Clean up allocated data in singleScene
            // (Note: It's a stack aiScene, but we allocated materials and meshes)
            delete singleScene.mMeshes[0];
            delete [] singleScene.mMeshes;
            delete singleScene.mMaterials[0];
            delete [] singleScene.mMaterials;
            delete [] singleScene.mRootNode->mMeshes;
            delete singleScene.mRootNode;
        }
    } else {
        // Combined scene (either single frame or multiple frames in one file)
        size_t meshCount = frameCount;
        scene->mMeshes = new aiMesh*[meshCount];
        scene->mMaterials = new aiMaterial*[meshCount];
        scene->mNumMeshes = (unsigned int)meshCount;
        scene->mNumMaterials = (unsigned int)(separateTexturesPerMesh ? meshCount : 1);
        if(separateTexturesPerMesh) {
            // each mesh gets its own material
            for(size_t i = 0; i < meshCount; ++i) {
                scene->mMaterials[i] = new aiMaterial();
            }
        } else {
            // one material for all meshes
            scene->mMaterials[0] = new aiMaterial();
            for(size_t i = 1; i < meshCount; ++i) {
                scene->mMaterials[i] = scene->mMaterials[0];
            }
        }
        // Root node children for each mesh
        scene->mRootNode->mNumChildren = (unsigned int)meshCount;
        scene->mRootNode->mChildren = new aiNode*[meshCount];
        // Iterate through frames
        int globalAtlasSize = 0;
        std::vector<unsigned char> globalImage;
        if(!separateTexturesPerMesh) {
            // If one atlas for all, gather all faces first
            std::vector<FaceRect> allFaces;
            std::unordered_set<uint8_t> usedColors;
            for(size_t i = 0; i < meshCount; ++i) {
                size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size()-1);
                std::vector<FaceRect> faces = GreedyMeshModel(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], usedColors);
                // Tag faces with an offset or id if needed (not needed for atlas, we just combine)
                allFaces.insert(allFaces.end(), faces.begin(), faces.end());
            }
            // Pack combined faces
            int dim = 16;
            if(forcePowerOfTwo) {
                while(true) {
                    if(PackFacesIntoAtlas(dim, allFaces)) break;
                    dim *= 2;
                    if(dim > 8192) break;
                }
            } else {
                while(true) {
                    if(PackFacesIntoAtlas(dim, allFaces)) break;
                    dim += 16;
                    if(dim > 8192) break;
                }
                // Shrink to used extents
                int usedW = 0, usedH = 0;
                for(auto& fr : allFaces) {
                    usedW = std::max(usedW, fr.atlasX + fr.w + 2);
                    usedH = std::max(usedH, fr.atlasY + fr.h + 2);
                }
                dim = std::max(usedW, usedH);
            }
            globalAtlasSize = dim;
            GenerateAtlasImage(dim, dim, allFaces, voxData->palette, globalImage);
            // Save atlas image
            std::string baseName = outputPath;
            if(dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
            std::string atlasName = baseName + "_atlas.png";
            SaveAtlasImage(atlasName, dim, dim, globalImage);
            // Assign this texture to the single material
            aiString texPath(atlasName);
            scene->mMaterials[0]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
            // Now need to build each mesh's geometry from the portion of faces belonging to that mesh
            // We should separate faces by frame segment: because allFaces is combined list.
            // We can reconstruct segmentation because we processed each frame sequentially and appended.
            // So we can do another loop now generating geometry for each frame from allFaces:
            size_t faceOffset = 0;
            for(size_t i = 0; i < meshCount; ++i) {
                size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size()-1);
                // Remesh the frame to get number of faces:
                std::unordered_set<uint8_t> dummy;
                std::vector<FaceRect> frameFaces = GreedyMeshModel(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], dummy);
                // Now copy that many faces from allFaces (they should correspond in order to this frame).
                std::vector<FaceRect> facesForMesh;
                facesForMesh.insert(facesForMesh.end(), allFaces.begin() + faceOffset, allFaces.begin() + faceOffset + frameFaces.size());
                faceOffset += frameFaces.size();
                // Create mesh
                aiMesh* mesh = new aiMesh();
                scene->mMeshes[i] = mesh;
                BuildMeshFromFaces(facesForMesh, globalAtlasSize, globalAtlasSize, flatShading, voxData->palette, mesh);
                mesh->mMaterialIndex = separateTexturesPerMesh ? (int)i : 0;
                // Create node for this mesh
                aiNode* node = new aiNode();
                node->mName = aiString("Frame" + std::to_string(i));
                node->mNumMeshes = 1;
                node->mMeshes = new unsigned int[1];
                node->mMeshes[0] = i;
                node->mTransformation = aiMatrix4x4(); // identity (we place all frames at origin overlapping)
                scene->mRootNode->mChildren[i] = node;
            }
        } else {
            // separateTexturesPerMesh case:
            for(size_t i = 0; i < meshCount; ++i) {
                size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size()-1);
                std::unordered_set<uint8_t> used;
                std::vector<FaceRect> faces = GreedyMeshModel(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], used);
                int dim = 16;
                if(forcePowerOfTwo) {
                    while(true) {
                        if(PackFacesIntoAtlas(dim, faces)) break;
                        dim *= 2;
                        if(dim > 4096) break;
                    }
                } else {
                    while(true) {
                        if(PackFacesIntoAtlas(dim, faces)) break;
                        dim += 16;
                        if(dim > 4096) break;
                    }
                    // shrink
                    int usedW = 0, usedH = 0;
                    for(auto& fr : faces) {
                        usedW = std::max(usedW, fr.atlasX + fr.w + 2);
                        usedH = std::max(usedH, fr.atlasY + fr.h + 2);
                    }
                    dim = std::max(usedW, usedH);
                }
                // Create atlas for this mesh
                std::vector<unsigned char> img;
                GenerateAtlasImage(dim, dim, faces, voxData->palette, img);
                std::string baseName = outputPath;
                if(dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
                std::string imageName = baseName + "_mesh" + std::to_string(i) + ".png";
                SaveAtlasImage(imageName, dim, dim, img);
                aiString texPath(imageName);
                scene->mMaterials[i]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
                // Create mesh geometry
                aiMesh* mesh = new aiMesh();
                scene->mMeshes[i] = mesh;
                BuildMeshFromFaces(faces, dim, dim, flatShading, voxData->palette, mesh);
                mesh->mMaterialIndex = (int)i;
                // Node
                aiNode* node = new aiNode();
                node->mName = aiString("Mesh" + std::to_string(i));
                node->mNumMeshes = 1;
                node->mMeshes = new unsigned int[1];
                node->mMeshes[0] = i;
                node->mTransformation = aiMatrix4x4(); // identity (assuming all at origin)
                scene->mRootNode->mChildren[i] = node;
            }
        }
        // If there was only one mesh in scene (no children used above), attach it directly to root node
        if(meshCount == 1) {
            scene->mRootNode->mNumMeshes = 1;
            scene->mRootNode->mMeshes = new unsigned int[1];
            scene->mRootNode->mMeshes[0] = 0;
        }
        // Export combined scene
        if(!exportScene(outputPath)) {
            std::cerr << "Failed to export scene.\n";
            return 1;
        } else {
            std::cout << "Exported " << outputPath << " successfully.\n";
        }
    }

    // Clean up dynamically allocated scene data for combined case
    if(!exportFramesSeparately) {
        // Clean materials (if unique)
        std::unordered_set<aiMaterial*> uniqueMats;
        for(unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            uniqueMats.insert(scene->mMaterials[i]);
        }
        for(aiMaterial* mat : uniqueMats) {
            delete mat;
        }
        // Clean meshes
        for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            delete scene->mMeshes[i];
        }
        delete [] scene->mMeshes;
        delete [] scene->mMaterials;
        // Clean nodes
        for(unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i) {
            aiNode* child = scene->mRootNode->mChildren[i];
            delete [] child->mMeshes;
            delete child;
        }
        if(scene->mRootNode->mMeshes) delete [] scene->mRootNode->mMeshes;
        delete scene->mRootNode;
        delete scene;
    }
    return 0;
}
