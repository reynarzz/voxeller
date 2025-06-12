#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <voxeller/GreedyTest.h>
#include <assimp/postprocess.h>
// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <voxeller/VoxParser.h>
#include <voxeller/Log/Log.h>

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


#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

struct MyTraits : public OpenMesh::DefaultTraits {
  VertexTraits {
    // store one 2D texcoord per vertex
    OpenMesh::Vec2f  texcoord_;
    OpenMesh::Vec3f  normal_;     // optionally store normal, too
  };
};
using TriMesh = OpenMesh::TriMesh_ArrayKernelT<MyTraits>;


// Import an aiMesh into OpenMesh
static void convertAiMeshToOpenMesh(aiMesh* aimesh, TriMesh& om) {
    om.clear();

    // Enable per‐vertex normals and UVs in OpenMesh
    om.request_vertex_normals();
    om.request_vertex_texcoords2D();

    // Keep a handle list for adding faces
    std::vector<TriMesh::VertexHandle> vhandle(aimesh->mNumVertices);

    // Import vertices, normals and UVs
    for (unsigned int i = 0; i < aimesh->mNumVertices; ++i) {
        // Position
        const aiVector3D& p  = aimesh->mVertices[i];
        vhandle[i] = om.add_vertex({ p.x, p.y, p.z });

        // Normal (if present)
        if (aimesh->HasNormals()) {
            const aiVector3D& n = aimesh->mNormals[i];
            om.set_normal(vhandle[i], { n.x, n.y, n.z });
        }

        // UV (if present)
        if (aimesh->HasTextureCoords(0)) {
            const aiVector3D& uv = aimesh->mTextureCoords[0][i];
            om.set_texcoord2D(vhandle[i], { uv.x, uv.y });
        }
    }

    // Import faces (triangles)
    for (unsigned int f = 0; f < aimesh->mNumFaces; ++f) {
        const aiFace& face = aimesh->mFaces[f];
        std::vector<TriMesh::VertexHandle> fv;
        fv.reserve(face.mNumIndices);
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            fv.push_back(vhandle[face.mIndices[j]]);
        }
        om.add_face(fv);
    }

    // Optionally update OpenMesh’s internal normals if you want to recompute:
    // om.update_normals();
}

// Export OpenMesh back into the existing aiMesh (overwriting its arrays)
// Converts an OpenMesh TriMesh (with per‐vertex normals and 2D texcoords) back into an existing aiMesh
static void convertOpenMeshToAiMesh(TriMesh& om, aiMesh* aimesh) {
    // Ensure the mesh has normals and texcoords available
    if (!om.has_vertex_normals()) {
        om.request_vertex_normals();
        om.update_normals();
    }
    if (!om.has_vertex_texcoords2D()) {
        om.request_vertex_texcoords2D();
        // You may need to compute or copy UVs before calling this
    }

    // --- Vertices, normals, texcoords ---
    const unsigned int nv = static_cast<unsigned int>(om.n_vertices());

    // Delete any existing data to avoid leaks
    delete[] aimesh->mVertices;
    delete[] aimesh->mNormals;
    delete[] aimesh->mTextureCoords[0];

    // Allocate new arrays
    aimesh->mNumVertices = nv;
    aimesh->mVertices    = new aiVector3D[nv];
    aimesh->mNormals     = new aiVector3D[nv];
    aimesh->mTextureCoords[0] = new aiVector3D[nv];
    aimesh->mNumUVComponents[0] = 2;  // 2D UVs

    // Map from OpenMesh vertex index → aiMesh index
    std::vector<unsigned int> idxMap(nv);
    unsigned int idx = 0;
    for (auto vh : om.vertices()) {
        // Position
        auto  p  = om.point(vh);
        aimesh->mVertices[idx] = aiVector3D(p[0], p[1], p[2]);

        // Normal
        auto  n  = om.normal(vh);
        aimesh->mNormals [idx] = aiVector3D(n[0], n[1], n[2]);

        // UV
        auto  uv = om.texcoord2D(vh);
        aimesh->mTextureCoords[0][idx] = aiVector3D(uv[0], uv[1], 0.0f);

        idxMap[vh.idx()] = idx++;
    }

    // --- Faces (triangles) ---
    const unsigned int nf = static_cast<unsigned int>(om.n_faces());
    delete[] aimesh->mFaces;
    aimesh->mNumFaces = nf;
    aimesh->mFaces    = new aiFace[nf];

    idx = 0;
    for (auto fh : om.faces()) {
        aiFace& af = aimesh->mFaces[idx];
        af.mNumIndices = 3;               // TriMesh_ArrayKernelT<> produces triangles
        af.mIndices    = new unsigned int[3];

        int vi = 0;
        for (auto fv_it = om.cfv_iter(fh); fv_it.is_valid(); ++fv_it) {
            af.mIndices[vi++] = idxMap[fv_it->idx()];
        }
        ++idx;
    }

    // Copy over primitive type
    aimesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
}

// Collapse any edge whose endpoints coincide (within eps):
static void collapseDuplicateVertices(TriMesh& mesh) {
  const float eps2 = 1e-12f;
  mesh.request_vertex_status();
  mesh.request_edge_status();
  mesh.request_face_status();

  for (auto eh : mesh.edges()) {
    auto heh = mesh.halfedge_handle(eh, 0);
    auto v0  = mesh.from_vertex_handle(heh);
    auto v1  = mesh.to_vertex_handle(heh);
    if ((mesh.point(v0) - mesh.point(v1)).sqrnorm() < eps2
        && mesh.is_collapse_ok(heh)) {
      mesh.collapse(heh);
    }
  }
}

// Split edges whenever a vertex lies on their interior (i.e. T-junction):
static void splitTJunctions(TriMesh& mesh) {
  const float eps2 = 1e-8f;   // squared distance threshold
  mesh.request_face_normals();
  mesh.request_vertex_normals();
  mesh.request_vertex_texcoords2D();

  for (auto vh : mesh.vertices()) {
    auto  p = mesh.point(vh);
    for (auto eh : mesh.edges()) {
      if (mesh.status(eh).deleted()) continue;
      auto  heh = mesh.halfedge_handle(eh,0);
      auto  v0  = mesh.from_vertex_handle(heh);
      auto  v1  = mesh.to_vertex_handle(heh);
      if (vh==v0 || vh==v1) continue;
      auto  p0  = mesh.point(v0);
      auto  p1  = mesh.point(v1);
      auto  dir = p1-p0;
      float d2  = dir.sqrnorm();
      float t   = ( (p-p0) | dir )/ d2;
      if (t>0.01f && t<0.99f) {
        auto proj = p0 + dir * t;
        if ((proj-p).sqrnorm() < eps2) {
          mesh.split(eh, vh);
          // after splitting, restart edge iteration on this vertex
          goto next_vertex;
        }
      }
    }
    next_vertex: ;
  }
}

static void CleanUpMesh(aiMesh* mesh)
{
    TriMesh om;
convertAiMeshToOpenMesh(mesh, om);

// 2) Weld duplicates + fix T-junctions
collapseDuplicateVertices(om);
splitTJunctions(om);

// 3) Clean up deletions
om.garbage_collection();

// 4) Recompute smooth normals
om.request_face_normals();
om.request_vertex_normals();
om.update_normals();

// 5) Convert OpenMesh → aiMesh
convertOpenMeshToAiMesh(om, mesh);
}

static std::vector<FaceRect> GreedyMeshModel(
    const vox_model& model,
    const vox_size&  size, std::unordered_set<uint8_t>& usedColors)
{
    const int X = size.x;
    const int Y = size.y;
    const int Z = size.z;
    std::vector<FaceRect> faces;
    faces.reserve(1000);

    // Quick occupancy test
    auto isFilled = [&](int x,int y,int z){
        if(x<0||x>=X||y<0||y>=Y||z<0||z>=Z) return false;
        return model.voxel_3dGrid[z][y][x] >= 0;
    };

    // Helper lambda to do one 2D‐greedy pass:
    auto sweep = [&](char orient,
                     int dimU, int dimV, int dimW,
                     auto getFilled,
                     auto getPlaneConst)
    {
        // dimU,dimV = extents of the mask; dimW = sweep axis length
        std::vector<bool> mask(dimU*dimV), visited(dimU*dimV);
        for(int w=0; w<dimW; ++w){
            // clear mask+visited
            std::fill(mask.begin(),    mask.end(),    false);
            std::fill(visited.begin(), visited.end(), false);

            // build mask[u,v] = true if face at (u,v,w)
            for(int v=0; v<dimV; ++v) for(int u=0; u<dimU; ++u){
                if(getFilled(u,v,w))
                    mask[v*dimU + u] = true;
            }

            // greedy‐merge rects in mask
            for(int v=0; v<dimV; ++v) for(int u=0; u<dimU; ++u){
                int idx = v*dimU + u;
                if(!mask[idx] || visited[idx]) continue;

                // expand width
                int wU = 1;
                while(u+wU<dimU && mask[v*dimU + (u+wU)] 
                                  && !visited[v*dimU + (u+wU)])
                    ++wU;

                // expand height
                int wV = 1;
                bool ok=true;
                while(ok && v+wV<dimV){
                    for(int k=0; k<wU; ++k){
                        int idx2 = (v+wV)*dimU + (u+k);
                        if(!mask[idx2] || visited[idx2]){
                            ok=false; break;
                        }
                    }
                    if(ok) ++wV;
                }

                // mark visited
                for(int dv=0; dv<wV; ++dv)
                    for(int du=0; du<wU; ++du)
                        visited[(v+dv)*dimU + (u+du)] = true;

                // record a FaceRect
                FaceRect f;
                f.orientation   = orient;
                f.constantCoord = getPlaneConst(u,v,w);
                f.uMin = u;     f.uMax = u + wU;
                f.vMin = v;     f.vMax = v + wV;
                f.w    = wU;    f.h    = wV;
                f.colorIndex = 0;            // unused for merging
                faces.push_back(f);
            }
        }
    };

    // +X ('X'): sweep w=x in [0..X-1], UV=(z,y)
    sweep('X',
          /*dimU=*/Z, /*dimV=*/Y, /*dimW=*/X,
          [&](int z,int y,int x){
             return isFilled(x,y,z) && (x==X-1 || !isFilled(x+1,y,z));
          },
          [&](int z,int y,int x){
             return x+1; // plane at x+1
          });

    // -X ('x'): sweep w=x, UV=(z,y)
    sweep('x',
          Z, Y, X,
          [&](int z,int y,int x){
             return isFilled(x,y,z) && (x==0 || !isFilled(x-1,y,z));
          },
          [&](int z,int y,int x){
             return x;   // plane at x
          });

    // +Y ('Y'): sweep w=y, UV=(x,z)
    sweep('Y',
          X, Z, Y,
          [&](int x,int z,int y){
             return isFilled(x,y,z) && (y==Y-1 || !isFilled(x,y+1,z));
          },
          [&](int x,int z,int y){
             return y+1;
          });

    // -Y ('y'): sweep w=y, UV=(x,z)
    sweep('y',
          X, Z, Y,
          [&](int x,int z,int y){
             return isFilled(x,y,z) && (y==0   || !isFilled(x,y-1,z));
          },
          [&](int x,int z,int y){
             return y;
          });

    // +Z ('Z'): sweep w=z, UV=(x,y)
    sweep('Z',
          X, Y, Z,
          [&](int x,int y,int z){
             return isFilled(x,y,z) && (z==Z-1 || !isFilled(x,y,z+1));
          },
          [&](int x,int y,int z){
             return z+1;
          });

    // -Z ('z'): sweep w=z, UV=(x,y)
    sweep('z',
          X, Y, Z,
          [&](int x,int y,int z){
             return isFilled(x,y,z) && (z==0   || !isFilled(x,y,z-1));
          },
          [&](int x,int y,int z){
             return z;
          });

    return faces;
}

// Performs greedy meshing on a single vox_model to extract faces and merge contiguous ones.
// Returns a list of FaceRect representing all merged faces (with orientation and extents), and sets usedColors (unique palette indices used).
static std::vector<FaceRect> GreedyMeshModel_(const vox_model& model, const vox_size& size, std::unordered_set<uint8_t>& usedColors) {
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
static void BuildMeshFromFaces(
    const std::vector<FaceRect>& faces,
    int texWidth,
    int texHeight,
    bool flatShading,
    const std::vector<color>& palette,
    aiMesh* mesh)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(faces.size() * 4);
    indices.reserve(faces.size() * 6);

    // map<quantized pos+uv, index> for smooth shading
    std::unordered_map<VertKey, unsigned int, VertKeyHash> vertMap;
    vertMap.reserve(faces.size() * 4);

    auto addVertex = [&](float vx, float vy, float vz,
                         float nx, float ny, float nz,
                         float u, float v) -> unsigned int
    {
            VertKey key;

        if (!flatShading) {
            key.px_i = int(std::round(vx * 100.0f));
            key.py_i = int(std::round(vy * 100.0f));
            key.pz_i = int(std::round(vz * 100.0f));
            key.u_i  = int(std::round(u  * texWidth  * 100.0f));
            key.v_i  = int(std::round(v  * texHeight * 100.0f));
            auto it = vertMap.find(key);
            if (it != vertMap.end()) {
                unsigned int idx = it->second;
                vertices[idx].nx += nx;
                vertices[idx].ny += ny;
                vertices[idx].nz += nz;
                return idx;
            }
            // else fall through and create new
        }
        Vertex vert;
        vert.px = vx; vert.py = vy; vert.pz = vz;
        vert.nx = nx; vert.ny = ny; vert.nz = nz;
        vert.u  = u;  vert.v  = v;
        unsigned int idx = vertices.size();
        vertices.push_back(vert);
        if (!flatShading) vertMap[key] = idx;
        return idx;
    };

    // Precompute some atlas‐space constants
    const float pixelW = 1.0f / float(texWidth);
    const float pixelH = 1.0f / float(texHeight);
    const float border = 1.0f;

    for (auto const& face : faces) {
        // 1px bleed → inset by 0.5px to sample at centers
        float u0 = (face.atlasX + border + 0.5f)           * pixelW;
        float v0 = 1.0f - (face.atlasY + border + 0.5f)    * pixelH;
        float u1 = (face.atlasX + border + face.w - 0.5f)  * pixelW;
        float v1 = 1.0f - (face.atlasY + border + face.h - 0.5f) * pixelH;

        // determine face normal & 3D‐coords for corners
        float nx=0, ny=0, nz=0;
        float x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3;

        switch (face.orientation) {
          case 'X': {
            nx = +1; 
            float fx = float(face.constantCoord);
            float zmin = float(face.uMin), zmax = float(face.uMax);
            float ymin = float(face.vMin), ymax = float(face.vMax);
            // bottom‐left
            x0=fx; y0=ymin; z0=zmin;
            // top‐left
            x1=fx; y1=ymax; z1=zmin;
            // top‐right
            x2=fx; y2=ymax; z2=zmax;
            // bottom‐right
            x3=fx; y3=ymin; z3=zmax;
            } break;
          case 'x': {
            nx = -1;
            float fx = float(face.constantCoord);
            float zmin = float(face.uMin), zmax = float(face.uMax);
            float ymin = float(face.vMin), ymax = float(face.vMax);
            // reverse U‐axis so pattern stays upright
            std::swap(zmin,zmax);
            x0=fx; y0=ymin; z0=zmin;
            x1=fx; y1=ymax; z1=zmin;
            x2=fx; y2=ymax; z2=zmax;
            x3=fx; y3=ymin; z3=zmax;
            } break;
          case 'Y': {
            ny = +1;
            float fy = float(face.constantCoord);
            float xmin = float(face.uMin), xmax = float(face.uMax);
            float zmin = float(face.vMin), zmax = float(face.vMax);
            x0=xmin; y0=fy; z0=zmin;
            x1=xmin; y1=fy; z1=zmax;
            x2=xmax; y2=fy; z2=zmax;
            x3=xmax; y3=fy; z3=zmin;
            } break;
          case 'y': {
            ny = -1;
            float fy = float(face.constantCoord);
            float xmin = float(face.uMin), xmax = float(face.uMax);
            float zmin = float(face.vMin), zmax = float(face.vMax);
            std::swap(zmin,zmax);
            x0=xmin; y0=fy; z0=zmin;
            x1=xmin; y1=fy; z1=zmax;
            x2=xmax; y2=fy; z2=zmax;
            x3=xmax; y3=fy; z3=zmin;
            } break;
          case 'Z': {
            nz = +1;
            float fz = float(face.constantCoord);
            float xmin = float(face.uMin), xmax = float(face.uMax);
            float ymin = float(face.vMin), ymax = float(face.vMax);
            x0=xmin; y0=ymin; z0=fz;
            x1=xmin; y1=ymax; z1=fz;
            x2=xmax; y2=ymax; z2=fz;
            x3=xmax; y3=ymin; z3=fz;
            } break;
          case 'z': {
            nz = -1;
            float fz = float(face.constantCoord);
            float xmin = float(face.uMin), xmax = float(face.uMax);
            float ymin = float(face.vMin), ymax = float(face.vMax);
            std::swap(xmin,xmax);
            x0=xmin; y0=ymin; z0=fz;
            x1=xmin; y1=ymax; z1=fz;
            x2=xmax; y2=ymax; z2=fz;
            x3=xmax; y3=ymin; z3=fz;
            } break;
          default: continue;
        }

        // add the four verts (always using the same u0,v0→u1,v1)
        unsigned int i0 = addVertex(x0,y0,z0, nx,ny,nz, u0,v0);
        unsigned int i1 = addVertex(x1,y1,z1, nx,ny,nz, u0,v1);
        unsigned int i2 = addVertex(x2,y2,z2, nx,ny,nz, u1,v1);
        unsigned int i3 = addVertex(x3,y3,z3, nx,ny,nz, u1,v0);

        indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
        indices.push_back(i0); indices.push_back(i2); indices.push_back(i3);
    }

    // normalize normals if smooth
    if (!flatShading) {
        for (auto& v : vertices) {
            float L = std::sqrt(v.nx*v.nx + v.ny*v.ny + v.nz*v.nz);
            if (L>0) { v.nx/=L; v.ny/=L; v.nz/=L; }
        }
    }

    // finally write into aiMesh
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    mesh->mNumVertices   = vertices.size();
    mesh->mVertices      = new aiVector3D[vertices.size()];
    mesh->mNormals       = new aiVector3D[vertices.size()];
    mesh->mTextureCoords[0] = new aiVector3D[vertices.size()];
    mesh->mNumUVComponents[0] = 2;

    for (unsigned int i=0;i<vertices.size();++i) {
        mesh->mVertices[i] = aiVector3D(vertices[i].px, vertices[i].py, vertices[i].pz);
        mesh->mNormals[i]  = aiVector3D(vertices[i].nx, vertices[i].ny, vertices[i].nz);
        mesh->mTextureCoords[0][i] = aiVector3D(vertices[i].u, vertices[i].v, 0.0f);
    }

    mesh->mNumFaces = indices.size()/3;
    mesh->mFaces    = new aiFace[mesh->mNumFaces];
    for (unsigned int f=0; f<mesh->mNumFaces; ++f) {
        aiFace& face = mesh->mFaces[f];
        face.mNumIndices = 3;
        face.mIndices    = new unsigned int[3];
        face.mIndices[0] = indices[f*3+0];
        face.mIndices[1] = indices[f*3+1];
        face.mIndices[2] = indices[f*3+2];
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
static void GenerateAtlasImage(
    int texWidth,
    int texHeight,
    const std::vector<FaceRect>& faces,
    const vox_model& model,
    const std::vector<color>& palette,
    std::vector<unsigned char>& outImage)
{
    const int border = 1;
    outImage.assign(texWidth * texHeight * 4, 0);

    // Helper to fetch RGBA from palette given a MagicaVoxel colorIndex (1–255)
    auto getRGBA = [&](uint8_t ci){
        size_t idx = ci > 0 ? ci - 1 : 0;
        if (idx >= palette.size()) idx = palette.size() - 1;
        return palette[idx];
    };

    // Sample the colorIndex at a given (x,y,z)
    auto sampleCI = [&](int x, int y, int z)->uint8_t {
        int cell = model.voxel_3dGrid[z][y][x];
        if (cell < 0) return 0;
        return model.voxels[cell].colorIndex;
    };

    for (auto& face : faces) {
        int x0 = face.atlasX;
        int y0 = face.atlasY;
        int w  = face.w;
        int h  = face.h;

        // 1) Fill interior (w×h) by sampling the original voxels
        for (int iy = 0; iy < h; ++iy) {
            for (int ix = 0; ix < w; ++ix) {
                int vx, vy, vz;
                switch (face.orientation) {
                  case 'X': // +X face: plane at x+1, u→Z, v→Y
                    vx = face.constantCoord - 1;
                    vy = face.vMin + iy;
                    vz = face.uMin + ix;
                    break;
                  case 'x': // -X face: plane at x,  u→←Z, v→Y
                    vx = face.constantCoord;
                    vy = face.vMin + iy;
                    vz = face.uMin + (w - 1 - ix);
                    break;
                  case 'Y': // +Y face: plane at y+1, u→X, v→Z
                    vx = face.uMin + ix;
                    vy = face.constantCoord - 1;
                    vz = face.vMin + iy;
                    break;
                  case 'y': // -Y face: plane at y,  u→X, v→←Z
                    vx = face.uMin + ix;
                    vy = face.constantCoord;
                    vz = face.vMin + (h - 1 - iy);
                    break;
                  case 'Z': // +Z face: plane at z+1, u→X, v→Y
                    vx = face.uMin + ix;
                    vy = face.vMin + iy;
                    vz = face.constantCoord - 1;
                    break;
                  case 'z': // -Z face: plane at z,  u→←X, v→Y
                    vx = face.uMin + (w - 1 - ix);
                    vy = face.vMin + iy;
                    vz = face.constantCoord;
                    break;
                  default:
                    continue;
                }

                uint8_t ci = sampleCI(vx, vy, vz);
                auto col = getRGBA(ci);

                int px = x0 + border + ix;
                int py = y0 + border + iy;
                int idx = (py * texWidth + px) * 4;
                outImage[idx + 0] = col.r;
                outImage[idx + 1] = col.g;
                outImage[idx + 2] = col.b;
                outImage[idx + 3] = col.a;
            }
        }

        // 2) Bleed edges into the 1-pixel border on all four sides:

        // Top & bottom
        for (int ix = 0; ix < w; ++ix) {
            // copy from first interior row → top border
            int srcTop = ((y0 + border + 0) * texWidth + (x0 + border + ix)) * 4;
            int dstTop = ((y0 + 0)           * texWidth + (x0 + border + ix)) * 4;
            memcpy(&outImage[dstTop], &outImage[srcTop], 4);

            // copy from last interior row → bottom border
            int srcBot = ((y0 + border + h - 1) * texWidth + (x0 + border + ix)) * 4;
            int dstBot = ((y0 + border + h)     * texWidth + (x0 + border + ix)) * 4;
            memcpy(&outImage[dstBot], &outImage[srcBot], 4);
        }

        // Left & right
        for (int iy = 0; iy < h; ++iy) {
            // copy from first interior column → left border
            int srcL = ((y0 + border + iy) * texWidth + (x0 + border + 0)) * 4;
            int dstL = ((y0 + border + iy) * texWidth + (x0 + 0))           * 4;
            memcpy(&outImage[dstL], &outImage[srcL], 4);

            // copy from last interior column → right border
            int srcR = ((y0 + border + iy) * texWidth + (x0 + border + w - 1)) * 4;
            int dstR = ((y0 + border + iy) * texWidth + (x0 + border + w))     * 4;
            memcpy(&outImage[dstR], &outImage[srcR], 4);
        }

        // Finally fill corners by copying the appropriate interior pixel:
        // top-left corner
        memcpy(
          &outImage[( (y0+0)       * texWidth + (x0+0)      )*4],
          &outImage[( (y0+border)  * texWidth + (x0+border) )*4],
          4
        );
        // top-right
        memcpy(
          &outImage[( (y0+0)       * texWidth + (x0+border+w) )*4],
          &outImage[( (y0+border)  * texWidth + (x0+border+w-1) )*4],
          4
        );
        // bottom-left
        memcpy(
          &outImage[( (y0+border+h)* texWidth + (x0+0)      )*4],
          &outImage[( (y0+border+h-1)*texWidth + (x0+border) )*4],
          4
        );
        // bottom-right
        memcpy(
          &outImage[( (y0+border+h)* texWidth + (x0+border+w) )*4],
          &outImage[( (y0+border+h-1)*texWidth + (x0+border+w-1) )*4],
          4
        );
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
        exportFramesSeparately = true;
    }
    // Ask user about atlas vs separate textures if multiple meshes will be in one file
    bool separateTexturesPerMesh = false;
    if(!exportFramesSeparately && frameCount > 1) {
        std::cout << "Use separate texture per frame/mesh instead of one atlas? (y/n): ";
       
        separateTexturesPerMesh = false;
    }
    // Ask about power-of-two texture enforcement
    bool forcePowerOfTwo = true;
    std::cout << "Force texture dimensions to power-of-two? (y/n): ";
   
    forcePowerOfTwo = true;
    // Ask about shading (flat or smooth)
    bool flatShading = true;
    std::cout << "Flat shading (y) or smooth shading (n)? ";
    flatShading = true;

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
        aiReturn ret = exporter.Export(scene, formatId.c_str(), outName, aiProcess_JoinIdenticalVertices);
        if(ret != aiReturn_SUCCESS) {
            std::cerr << "Export failed: " << exporter.GetErrorString() << "\n";
            return false;
        }
        return true;
    };
LOG_EDITOR_INFO("Frames count: {0}, Models: {1}", frameCount, voxData->voxModels.size());
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
            GenerateAtlasImage(atlasDim, atlasDim, faces,voxData->voxModels[modelIndex], voxData->palette, image);
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
            //GenerateAtlasImage(dim, dim, allFaces, voxData->palette, globalImage);
            GenerateAtlasImage(dim, dim, allFaces,voxData->voxModels[0], voxData->palette, globalImage);

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
                aiMatrix4x4 rot;
                aiMatrix4x4::RotationX(-static_cast<float>(AI_MATH_PI/2.0f), rot);
                node->mTransformation = rot;

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
                //GenerateAtlasImage(dim, dim, faces, voxData->palette, img);
            GenerateAtlasImage(dim, dim, faces,voxData->voxModels[modelIndex], voxData->palette, img);

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
                 aiMatrix4x4 rot;
                aiMatrix4x4::RotationX(-static_cast<float>(AI_MATH_PI/2.0f), rot);
                node->mTransformation = rot;

                scene->mRootNode->mChildren[i] = node;
            }
        }
        // If there was only one mesh in scene (no children used above), attach it directly to root node
        
for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
    CleanUpMesh(scene->mMeshes[m]);
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
        // for(unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        //     delete scene->mMeshes[i];
        // }
        // delete [] scene->mMeshes;
        // delete [] scene->mMaterials;
        // // Clean nodes
        // for(unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i) {
        //     aiNode* child = scene->mRootNode->mChildren[i];
        //     delete [] child->mMeshes;
        //     delete child;
        // }
        // if(scene->mRootNode->mMeshes) delete [] scene->mRootNode->mMeshes;
        // delete scene->mRootNode;
        // delete scene;
    }
    return 0;
}
