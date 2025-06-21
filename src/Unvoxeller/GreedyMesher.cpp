#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <algorithm>
#include <cassert>
#include <Unvoxeller/GreedyMesher.h>
#include <assimp/postprocess.h>
// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <Unvoxeller/VoxParser.h>
#include <Unvoxeller/Log/Log.h>
#include <Unvoxeller/VertexMerger.h>
#include <meshoptimizer/src/meshoptimizer.h>


// Include stb_image_write for saving texture atlas as PNG
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// Assume the Unvoxeller namespace and structures from the provided data structure are available:
using namespace Unvoxeller;

// A simple structure for 3D vertex with position, normal, UV
//struct Vertex
//{
//	float px, py, pz;
//	float nx, ny, nz;
//	float u, v;
//};

// Key for hashing vertex position+UV to merge vertices (for smooth shading)
//struct VertKey
//{
//	int px_i, py_i, pz_i;    // quantized position (as integers to avoid float issues)
//	int u_i, v_i;            // quantized UV (as integers in texture pixel space)
//	// Note: We quantize UV by multiplying by texture width/height when comparing keys.
//	bool operator==(const VertKey& other) const
//	{
//		return px_i == other.px_i && py_i == other.py_i && pz_i == other.pz_i
//			&& u_i == other.u_i && v_i == other.v_i;
//	}
//};

//// Hash for VertKey
//struct VertKeyHash
//{
//	size_t operator()(const VertKey& key) const noexcept
//	{
//		// combine the components into one hash (using 64-bit to avoid overflow)
//		uint64_t hash = ((uint64_t)(key.px_i * 73856093) ^ (uint64_t)(key.py_i * 19349663) ^ (uint64_t)(key.pz_i * 83492791));
//		hash ^= ((uint64_t)(key.u_i * 4256249) ^ (uint64_t)(key.v_i * 253 ^ (hash >> 32)));
//		return (size_t)hash;
//	}
//};

// Structure to hold a face (rectangle) that needs to be packed into the atlas
struct FaceRect
{
	int w, h;        // dimensions (including border will be added around)
	int atlasX, atlasY; // top-left position in atlas (including border) after packing
	uint8_t colorIndex; // palette index (for color)
	// face orientation and extents for UV mapping
	char orientation; // one of: 'X','x','Y','y','Z','z' for +X, -X, +Y, -Y, +Z, -Z
	// Face extents in voxel coordinates (the *inclusive* start and end boundaries of the face in world coordinates)
	int uMin, uMax;  // min and max along face's U-axis (in world coords, face boundary)
	int vMin, vMax;  // min and max along face's V-axis
	int constantCoord; // the coordinate of the face plane (e.g., x or y or z value for the face)
	s32 modelIndex;
};


#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <stack>

struct MyTraits : public OpenMesh::DefaultTraits {
	// turn on per-vertex normals and 2D texcoords
	VertexAttributes(OpenMesh::Attributes::Normal |
		OpenMesh::Attributes::TexCoord2D);
};
using TriMesh = OpenMesh::TriMesh_ArrayKernelT<MyTraits>;
#include <Unvoxeller/TjunctionsFixer.h>


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
		const aiVector3D& p = aimesh->mVertices[i];
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
static void convertOpenMeshToAiMesh(TriMesh& om, aiMesh* aimesh)
{
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
	aimesh->mVertices = new aiVector3D[nv];
	aimesh->mNormals = new aiVector3D[nv];
	aimesh->mTextureCoords[0] = new aiVector3D[nv];
	aimesh->mNumUVComponents[0] = 2;  // 2D UVs

	// Map from OpenMesh vertex index → aiMesh index
	std::vector<unsigned int> idxMap(nv);
	unsigned int idx = 0;
	for (auto vh : om.vertices()) {
		// Position
		auto  p = om.point(vh);
		aimesh->mVertices[idx] = aiVector3D(p[0], p[1], p[2]);

		// Normal
		auto  n = om.normal(vh);
		aimesh->mNormals[idx] = aiVector3D(n[0], n[1], n[2]);

		// UV
		auto  uv = om.texcoord2D(vh);
		aimesh->mTextureCoords[0][idx] = aiVector3D(uv[0], uv[1], 0.0f);

		idxMap[vh.idx()] = idx++;
	}

	// --- Faces (triangles) ---
	const unsigned int nf = static_cast<unsigned int>(om.n_faces());
	delete[] aimesh->mFaces;
	aimesh->mNumFaces = nf;
	aimesh->mFaces = new aiFace[nf];

	idx = 0;
	for (auto fh : om.faces()) {
		aiFace& af = aimesh->mFaces[idx];
		af.mNumIndices = 3;               // TriMesh_ArrayKernelT<> produces triangles
		af.mIndices = new unsigned int[3];

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
static void collapseDuplicateVertices(TriMesh& mesh)
{
	const float eps2 = 1e-12f;
	mesh.request_vertex_status();
	mesh.request_edge_status();
	mesh.request_face_status();

	for (auto eh : mesh.edges())
	{
		auto heh = mesh.halfedge_handle(eh, 0);
		auto v0 = mesh.from_vertex_handle(heh);
		auto v1 = mesh.to_vertex_handle(heh);

		if ((mesh.point(v0) - mesh.point(v1)).sqrnorm() < eps2 && mesh.is_collapse_ok(heh))
		{
			mesh.collapse(heh);
		}
	}
}

// Split edges whenever a vertex lies on their interior (i.e. T-junction):
static void splitTJunctions(TriMesh& mesh)
{
	const float eps2 = 1e-8f;   // squared‐distance threshold

	// Request the attributes we need
	mesh.request_vertex_normals();
	mesh.request_vertex_texcoords2D();

	LOG_INFO("T Juntions Fix: vertex count: {0}", mesh.n_vertices());
	LOG_INFO("T Juntions Fix: edges count: {0}", mesh.n_edges());

	// Iterate every vertex in the mesh
	for (auto vh : mesh.vertices())
	{
		auto  p = mesh.point(vh);

		// For each edge, see if this vertex lies in its interior
		for (auto eh : mesh.edges())
		{
			if (mesh.status(eh).deleted()) continue;

			// Get endpoints of the edge
			auto heh = mesh.halfedge_handle(eh, 0);
			auto v0 = mesh.from_vertex_handle(heh);
			auto v1 = mesh.to_vertex_handle(heh);
			if (vh == v0 || vh == v1) continue;

			auto p0 = mesh.point(v0);
			auto p1 = mesh.point(v1);
			auto dir = p1 - p0;
			float d2 = dir.sqrnorm();
			if (d2 < 1e-12f) continue;  // avoid degenerate

			// Project p onto the line p0→p1
			float t = ((p - p0) | dir) / d2;
			if (t <= 0.0f || t >= 1.0f) continue;

			OpenMesh::Vec3f proj = p0 + dir * t;
			if ((proj - p).sqrnorm() > eps2) continue;

			// Fetch normals & UVs so we can interpolate them
			auto n0 = mesh.normal(v0);
			auto n1 = mesh.normal(v1);
			auto uv0 = mesh.texcoord2D(v0);
			auto uv1 = mesh.texcoord2D(v1);

			// *** THIS is the key ***
			// Call the overload that takes a Point:
			TriMesh::VertexHandle new_vh = mesh.split(eh, proj);

			// Now fill in the new vertex’s attributes
			mesh.set_normal(new_vh, n0 + (n1 - n0) * t);
			mesh.set_texcoord2D(new_vh, uv0 + (uv1 - uv0) * t);

			// since topology changed, you might want to restart the edge‐loop for this vh
			break;
		}
	}
}
static void CleanUpMesh(aiMesh* mesh)
{
	TriMesh om;
	convertAiMeshToOpenMesh(mesh, om);

	// 2) Weld duplicates + fix T-junctions
	collapseDuplicateVertices(om);
	splitTJunctions(om);
	//splitTJunctionsFast(om);

	// 3) Clean up deletions
	om.garbage_collection();

	// 4) Recompute smooth normals
	om.request_face_normals();
	om.request_vertex_normals();
	om.update_normals();

	// 5) Convert OpenMesh → aiMesh
	convertOpenMeshToAiMesh(om, mesh);
}

static std::vector<FaceRect> GreedyMesh_Atlas(
	const vox_model& model,
	const vox_size& size, s32 modelIndex)
{
	const int X = size.x;
	const int Y = size.y;
	const int Z = size.z;
	std::vector<FaceRect> faces;
	faces.reserve(1000);

	// Quick occupancy test
	auto isFilled = [&](int x, int y, int z) {
		if (x < 0 || x >= X || y < 0 || y >= Y || z < 0 || z >= Z) return false;
		return model.voxel_3dGrid[z][y][x] >= 0;
		};

	// Helper lambda to do one 2D‐greedy pass:
	auto sweep = [&](char orient,
		int dimU, int dimV, int dimW,
		auto getFilled,
		auto getPlaneConst)
		{
			// dimU,dimV = extents of the mask; dimW = sweep axis length
			std::vector<bool> mask(dimU * dimV), visited(dimU * dimV);
			for (int w = 0; w < dimW; ++w) {
				// clear mask+visited
				std::fill(mask.begin(), mask.end(), false);
				std::fill(visited.begin(), visited.end(), false);

				// build mask[u,v] = true if face at (u,v,w)
				for (int v = 0; v < dimV; ++v) {
					for (int u = 0; u < dimU; ++u) {
						if (getFilled(u, v, w))
							mask[v * dimU + u] = true;
					}
				}

				// improved greedy‐merge rects in mask
				for (int v = 0; v < dimV; ++v) {
					for (int u = 0; u < dimU; ++u) {
						int idx = v * dimU + u;
						if (!mask[idx] || visited[idx]) continue;

						// 1) Compute run‐lengths for each row starting at (u,v)
						std::vector<int> rowWidths;
						for (int dv = 0; dv < dimV - v; ++dv) {
							int run = 0;
							int base = (v + dv) * dimU + u;
							while (u + run < dimU
								&& mask[base + run]
								&& !visited[base + run]) {
								++run;
							}
							if (run == 0) break;
							rowWidths.push_back(run);
						}

						// 2) Pick height h that maximizes area = h * min(widths[0..h))
						int bestArea = 0, bestW = 0, bestH = 0;
						for (int h = 1; h <= (int)rowWidths.size(); ++h) {
							int wMin = *std::min_element(rowWidths.begin(),
								rowWidths.begin() + h);
							int area = wMin * h;
							if (area > bestArea) {
								bestArea = area;
								bestW = wMin;
								bestH = h;
							}
						}

						// 3) Mark visited
						for (int dv = 0; dv < bestH; ++dv) {
							for (int du = 0; du < bestW; ++du) {
								visited[(v + dv) * dimU + (u + du)] = true;
							}
						}

						// 4) Record a FaceRect
						FaceRect f;
						f.orientation = orient;
						f.constantCoord = getPlaneConst(u, v, w);
						f.uMin = u;            f.uMax = u + bestW;
						f.vMin = v;            f.vMax = v + bestH;
						f.w = bestW;        f.h = bestH;
						f.colorIndex = 0;      // unused for merging
						f.modelIndex = modelIndex;
						faces.push_back(f);
					}
				}
			}
		};

	// +X ('X'): sweep w=x in [0..X-1], UV=(z,y)
	sweep('X',
		/*dimU=*/Z, /*dimV=*/Y, /*dimW=*/X,
		[&](int z, int y, int x) {
			return isFilled(x, y, z)
				&& (x == X - 1 || !isFilled(x + 1, y, z));
		},
		[&](int z, int y, int x) {
			return x + 1; // plane at x+1
		});

	// -X ('x'): sweep w=x, UV=(z,y)
	sweep('x',
		Z, Y, X,
		[&](int z, int y, int x) {
			return isFilled(x, y, z)
				&& (x == 0 || !isFilled(x - 1, y, z));
		},
		[&](int z, int y, int x) {
			return x;   // plane at x
		});

	// +Y ('Y'): sweep w=y, UV=(x,z)
	sweep('Y',
		X, Z, Y,
		[&](int x, int z, int y) {
			return isFilled(x, y, z)
				&& (y == Y - 1 || !isFilled(x, y + 1, z));
		},
		[&](int x, int z, int y) {
			return y + 1;
		});

	// -Y ('y'): sweep w=y, UV=(x,z)
	sweep('y',
		X, Z, Y,
		[&](int x, int z, int y) {
			return isFilled(x, y, z)
				&& (y == 0 || !isFilled(x, y - 1, z));
		},
		[&](int x, int z, int y) {
			return y;
		});

	// +Z ('Z'): sweep w=z, UV=(x,y)
	sweep('Z',
		X, Y, Z,
		[&](int x, int y, int z) {
			return isFilled(x, y, z)
				&& (z == Z - 1 || !isFilled(x, y, z + 1));
		},
		[&](int x, int y, int z) {
			return z + 1;
		});

	// -Z ('z'): sweep w=z, UV=(x,y)
	sweep('z',
		X, Y, Z,
		[&](int x, int y, int z) {
			return isFilled(x, y, z)
				&& (z == 0 || !isFilled(x, y, z - 1));
		},
		[&](int x, int y, int z) {
			return z;
		});

	return faces;
}

static std::vector<FaceRect> QuadMesh_Atlas(
	const vox_model& model,
	const vox_size& size,
	int modelIndex)
{
	const int X = size.x, Y = size.y, Z = size.z;
	std::vector<FaceRect> faces;
	faces.reserve(1024);

	// shorthand test + fetch color-index
	auto getColor = [&](int x, int y, int z) -> int {
		if (x < 0 || x >= X || y < 0 || y >= Y || z < 0 || z >= Z) return -1;
		return model.voxel_3dGrid[z][y][x];
		};

	// one 2D sweep over a “mask” of size U×V at depth w
	auto sweep = [&](char orient,
		int dimU, int dimV, int dimW,
		auto mapUVtoXYZ_face,   // (u,v,w)->(x,y,z) of the face‐voxel
		auto mapUVtoXYZ_adj,    // (u,v,w)->(x,y,z) of the neighbor
		auto getPlaneConstXYZ)  // (u,v,w)->plane coordinate
		{
			std::vector<int>  mask(dimU * dimV), visited(dimU * dimV);
			for (int w = 0; w < dimW; ++w) {
				// build mask[u,v] = color idx if face exists, else -1
				for (int v = 0; v < dimV; ++v) {
					for (int u = 0; u < dimU; ++u) {
						auto [fx, fy, fz] = mapUVtoXYZ_face(u, v, w);
						auto [ax, ay, az] = mapUVtoXYZ_adj(u, v, w);
						int  cFace = getColor(fx, fy, fz);
						int  cAdj = getColor(ax, ay, az);
						mask[v * dimU + u] = (cFace >= 0 && cAdj < 0) ? cFace : -1;
						visited[v * dimU + u] = 0;
					}
				}

				// greedy‐merge equal‐color runs
				for (int v = 0; v < dimV; ++v) {
					for (int u = 0; u < dimU; ++u) {
						int idx0 = v * dimU + u;
						int color = mask[idx0];
						if (color < 0 || visited[idx0]) continue;

						// expand width (u→u+wU) while same color & unvisited
						int wU = 1;
						while (u + wU < dimU) {
							int idx1 = v * dimU + (u + wU);
							if (mask[idx1] != color || visited[idx1]) break;
							++wU;
						}

						// expand height (v→v+wV) as long as each row matches
						int wV = 1;
						bool ok;
						while (v + wV < dimV) {
							ok = true;
							for (int k = 0; k < wU; ++k) {
								int idx2 = (v + wV) * dimU + (u + k);
								if (mask[idx2] != color || visited[idx2]) {
									ok = false; break;
								}
							}
							if (!ok) break;
							++wV;
						}

						// mark visited
						for (int dv = 0; dv < wV; ++dv)
							for (int du = 0; du < wU; ++du)
								visited[(v + dv) * dimU + (u + du)] = 1;

						// emit one quad (FaceRect)
						FaceRect f;
						f.orientation = orient;
						f.constantCoord = getPlaneConstXYZ(u, v, w);
						f.uMin = u;  f.uMax = u + wU;
						f.vMin = v;  f.vMax = v + wV;
						f.w = wU;    f.h = wV;
						f.colorIndex = color;
						f.modelIndex = modelIndex;
						faces.push_back(f);
					}
				}
			}
		};

	// +X faces: at voxel (x,y,z) looking +X, UV=(z,y)
	sweep('X',
		/*U=Z*/Z, /*V=Y*/Y, /*W=X*/X,
		/*face*/ [](int z, int y, int x) { return std::tuple{ x,y,z }; },
		/*adj */ [](int z, int y, int x) { return std::tuple{ x + 1,y,z }; },
		/*plane*/ [](int z, int y, int x) { return x + 1; }
	);

	// -X faces: UV=(z,y)
	sweep('x', Z, Y, X,
		[](int z, int y, int x) { return std::tuple{ x,y,z };      },
		[](int z, int y, int x) { return std::tuple{ x - 1,y,z };    },
		[](int z, int y, int x) { return x; }
	);

	// +Y: UV=(x,z)
	sweep('Y', X, Z, Y,
		[](int x, int z, int y) { return std::tuple{ x,y,z };    },
		[](int x, int z, int y) { return std::tuple{ x,y + 1,z };  },
		[](int x, int z, int y) { return y + 1; }
	);

	// -Y: UV=(x,z)
	sweep('y', X, Z, Y,
		[](int x, int z, int y) { return std::tuple{ x,y,z };    },
		[](int x, int z, int y) { return std::tuple{ x,y - 1,z };  },
		[](int x, int z, int y) { return y; }
	);

	// +Z: UV=(x,y)
	sweep('Z', X, Y, Z,
		[](int x, int y, int z) { return std::tuple{ x,y,z };    },
		[](int x, int y, int z) { return std::tuple{ x,y,z + 1 };  },
		[](int x, int y, int z) { return z + 1; }
	);

	// -Z: UV=(x,y)
	sweep('z', X, Y, Z,
		[](int x, int y, int z) { return std::tuple{ x,y,z };    },
		[](int x, int y, int z) { return std::tuple{ x,y,z - 1 };  },
		[](int x, int y, int z) { return z; }
	);

	return faces;
}

// A simple shelf-bin packer for placing rectangles (with added border) into a square atlas of given dimension.
// Returns true and updates FaceRect atlas positions if successful, or false if not fitting.
template<typename T>
static bool PackFacesIntoAtlas(int atlasSize, T& rects)
{
	// Sort rectangles by height (descending) for better packing (larger first).
	std::sort(rects.begin(), rects.end(), [](const FaceRect& a, const FaceRect& b)
		{
			// compare (h+2) including border
			int ah = a.h + 2;
			int bh = b.h + 2;
			if (ah == bh) {
				// if heights equal, maybe sort by width as well
				return (a.w + 2) > (b.w + 2);
			}
			return ah > bh;
		});
	int currentX = 0;
	int currentY = 0;
	int currentRowHeight = 0;
	int usedHeight = 0;
	for (auto& face : rects) {
		int rw = face.w + 2; // rect width with border
		int rh = face.h + 2; // rect height with border
		if (rw > atlasSize || rh > atlasSize) {
			return false; // one rect too big to ever fit
		}
		if (currentX + rw > atlasSize) {
			// start new row
			currentY += currentRowHeight;
			currentX = 0;
			currentRowHeight = 0;
		}
		if (currentY + rh > atlasSize) {
			return false; // height overflow
		}
		// place this rect
		face.atlasX = currentX;
		face.atlasY = currentY;
		// update row
		currentX += rw;
		if (rh > currentRowHeight) {
			currentRowHeight = rh;
		}
		usedHeight = std::max(usedHeight, currentY + currentRowHeight);
		if (usedHeight > atlasSize) {
			return false;
		}
	}
	return true;
}

inline vox_mat3 decode_rotation(uint8_t r)
{
	int idx[3], sign[3];
	idx[0] = (r >> 0) & 0x3;
	idx[1] = (r >> 2) & 0x3;
	idx[2] = 3 - idx[0] - idx[1]; // (since idx[0], idx[1], idx[2] is a permutation of 0,1,2)
	sign[0] = ((r >> 4) & 0x1) ? -1 : 1;
	sign[1] = ((r >> 5) & 0x1) ? -1 : 1;
	sign[2] = ((r >> 6) & 0x1) ? -1 : 1;

	float m[3][3] = {};
	m[0][idx[0]] = sign[0];
	m[1][idx[1]] = sign[1];
	m[2][idx[2]] = sign[2];

	return
	{
		m[0][0], m[0][1], m[0][2],
		m[1][0], m[1][1], m[1][2],
		m[2][0], m[2][1], m[2][2]
	};
}


inline vox_transform AccumulateWorldTransform(
	int shapeNodeID,
	int frameIndex,
	const vox_file& voxData)
{
	// Helper: find the transform node whose childNodeID == nodeID
	auto findTransformByChild = [&](int childID) -> int {
		for (auto const& kv : voxData.transforms) {
			if (kv.second.childNodeID == childID)
				return kv.first;
		}
		return -1;
		};

	// 1) start at the transform that directly points to our shape
	int trnID = findTransformByChild(shapeNodeID);
	if (trnID < 0) {
		return vox_transform(); // identity
	}

	// 2) We'll walk up to the root, but we need to apply parent transforms *first*,
	//    so push them on a stack, then multiply in that order.
	std::stack<vox_transform> xfStack;

	while (trnID != -1) {
		auto it = voxData.transforms.find(trnID);
		if (it == voxData.transforms.end()) break;
		const vox_nTRN& trn = it->second;

		// clamp frameIndex
		int fidx = (frameIndex < trn.framesCount ? frameIndex : 0);
		const vox_frame_attrib& attr = trn.frameAttrib[fidx];

		xfStack.push(vox_transform(attr.rotation, attr.translation));

		// 3) find the *parent* transform of this nTRN:
		//    a) first, check explicit "_parent" attributes
		int parentID = -1;
		for (auto const& key : { "_parent", "_parent_id", "_parentID" }) {
			auto ait = trn.attributes.find(key);
			if (ait != trn.attributes.end()) {
				parentID = std::stoi(ait->second);
				break;
			}
		}

		//    b) if none, maybe this trn is listed in an nGRP — find the group,
		//       then find the nTRN whose child is *that* group.
		if (parentID < 0) {
			for (auto const& gkv : voxData.groups) {
				for (int cid : gkv.second.childrenIDs) {
					if (cid == trnID) {
						// gkv.first is the group node ID
						parentID = findTransformByChild(gkv.first);
						break;
					}
				}
				if (parentID >= 0) break;
			}
		}

		trnID = parentID;
	}

	// 4) Now multiply them in parent→child order
	vox_transform world; // identity
	while (!xfStack.empty()) {
		world = xfStack.top() * world;
		xfStack.pop();
	}
	return world;
}

inline aiVector3D crossProduct(const aiVector3D& a, const aiVector3D& b)
{
	return aiVector3D(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

struct VertexOpt
{
	float pos[3];
	float normal[3];
	float uv[2];
};

// Optimize all meshes in an aiScene using meshoptimizer
//void OptimizeAssimpScene(aiScene* scene) 
//{
//	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
//	{
//		aiMesh* mesh = scene->mMeshes[mi];
//
//		// 1) Build interleaved vertex buffer
//		std::vector<VertexOpt> vertices(mesh->mNumVertices);
//
//		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) 
//		{
//			// Position
//			vertices[i].pos[0] = mesh->mVertices[i].x;
//			vertices[i].pos[1] = mesh->mVertices[i].y;
//			vertices[i].pos[2] = mesh->mVertices[i].z;
//			// Normal (if present)
//			if (mesh->HasNormals()) {
//				vertices[i].normal[0] = mesh->mNormals[i].x;
//				vertices[i].normal[1] = mesh->mNormals[i].y;
//				vertices[i].normal[2] = mesh->mNormals[i].z;
//			}
//			else {
//				vertices[i].normal[0] = vertices[i].normal[1] = vertices[i].normal[2] = 0.0f;
//			}
//			// UV0 (if present)
//			if (mesh->mTextureCoords[0]) {
//				vertices[i].uv[0] = mesh->mTextureCoords[0][i].x;
//				vertices[i].uv[1] = mesh->mTextureCoords[0][i].y;
//			}
//			else {
//				vertices[i].uv[0] = vertices[i].uv[1] = 0.0f;
//			}
//		}
//
//		// 2) Build index buffer (assume triangles)
//		std::vector<uint32_t> indices;
//		indices.reserve(mesh->mNumFaces * 3);
//		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
//			const aiFace& face = mesh->mFaces[f];
//			if (face.mNumIndices == 3) {
//				indices.push_back(face.mIndices[0]);
//				indices.push_back(face.mIndices[1]);
//				indices.push_back(face.mIndices[2]);
//			}
//			// skip non-triangular faces
//		}
//
//		// 3) Weld (dedupe) vertices
//		std::vector<uint32_t> remap(mesh->mNumVertices);
//		size_t uniqueCount = meshopt_generateVertexRemap(
//			remap.data(), indices.data(), indices.size(),
//			vertices.data(), vertices.size(), sizeof(VertexOpt)
//		);
//
//		std::vector<VertexOpt> newVertices(uniqueCount);
//		std::vector<uint32_t> newIndices(indices.size());
//
//		meshopt_remapVertexBuffer(
//			newVertices.data(), vertices.data(), vertices.size(), sizeof(VertexOpt), remap.data()
//		);
//		meshopt_remapIndexBuffer(
//			newIndices.data(), indices.data(), indices.size(), remap.data()
//		);
//
//		size_t targetIndexCount = indices.size() * 0.5; /* e.g. indices.size() * 0.5 */;
//		float  maxError = 0.01f /* world‐space error threshold */;
//		meshopt_simplify(
//			newIndices.data(),   // dst indices
//			newIndices.data(),   // src indices
//			newIndices.size(),
//			reinterpret_cast<const float*>(newVertices.data()),
//			newVertices.size(),
//			sizeof(VertexOpt),
//			targetIndexCount,
//			maxError,
//			meshopt_SimplifyLockBorder  // optional: lock UV‐seams
//		);
//
//
//		// 4) Optimize for GPU caches & overdraw
//		meshopt_optimizeVertexCache(
//			newIndices.data(), newIndices.data(), newIndices.size(), uniqueCount
//		);
//
//		meshopt_optimizeOverdraw(
//			newIndices.data(),                           // destination indices
//			newIndices.data(),                           // source    indices
//			newIndices.size(),                           // index_count
//			reinterpret_cast<const float*>(newVertices.data()), // vertex_positions (pos.x of VertexOpt must be first)
//			newVertices.size(),                          // vertex_count
//			sizeof(VertexOpt),                           // vertex_positions_stride (bytes between each pos)
//			1.05f                                        // threshold
//		);
//
//		
//		size_t finalVertexCount = meshopt_optimizeVertexFetch(
//			newVertices.data(),   // destination vertex buffer
//			newIndices.data(),    // in/out index buffer
//			newIndices.size(),    // index_count
//			newVertices.data(),   // source vertex buffer
//			newVertices.size(),   // vertex_count
//			sizeof(VertexOpt)     // vertex_size (stride in bytes)
//		);
//		// 5) Write back to aiMesh (replace vertex & face arrays)
//		// Free old data
//		delete[] mesh->mVertices;
//		delete[] mesh->mNormals;
//		delete[] mesh->mTextureCoords[0];
//
//		// Allocate new arrays
//		mesh->mNumVertices = static_cast<unsigned int>(uniqueCount);
//		mesh->mVertices = new aiVector3D[mesh->mNumVertices];
//		mesh->mNormals = new aiVector3D[mesh->mNumVertices];
//		mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
//
//		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
//			mesh->mVertices[i] = aiVector3D(
//				newVertices[i].pos[0], newVertices[i].pos[1], newVertices[i].pos[2]
//			);
//			mesh->mNormals[i] = aiVector3D(
//				newVertices[i].normal[0], newVertices[i].normal[1], newVertices[i].normal[2]
//			);
//			mesh->mTextureCoords[0][i] = aiVector3D(
//				newVertices[i].uv[0], newVertices[i].uv[1], 0.0f
//			);
//		}
//
//		// Faces
//		size_t newFaceCount = newIndices.size() / 3;
//		delete[] mesh->mFaces;
//		mesh->mNumFaces = static_cast<unsigned int>(newFaceCount);
//		mesh->mFaces = new aiFace[mesh->mNumFaces];
//
//		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
//			aiFace& face = mesh->mFaces[f];
//			face.mNumIndices = 3;
//			face.mIndices = new unsigned int[3];
//			face.mIndices[0] = newIndices[f * 3 + 0];
//			face.mIndices[1] = newIndices[f * 3 + 1];
//			face.mIndices[2] = newIndices[f * 3 + 2];
//		}
//	}
//}

// New Optimizer
void OptimizeAssimpScene(aiScene* scene) 
{
	LOG_INFO("Optimizing mesh");

	for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi)
	{
		aiMesh* mesh = scene->mMeshes[mi];

		// 1) Build interleaved vertex buffer
		std::vector<VertexOpt> vertices(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) 
		{
			// Position
			vertices[i].pos[0] = mesh->mVertices[i].x;
			vertices[i].pos[1] = mesh->mVertices[i].y;
			vertices[i].pos[2] = mesh->mVertices[i].z;
			// Normal (if present)
			if (mesh->HasNormals()) {
				vertices[i].normal[0] = mesh->mNormals[i].x;
				vertices[i].normal[1] = mesh->mNormals[i].y;
				vertices[i].normal[2] = mesh->mNormals[i].z;
			}
			else {
				vertices[i].normal[0] = vertices[i].normal[1] = vertices[i].normal[2] = 0.0f;
			}
			// UV0 (if present)
			if (mesh->mTextureCoords[0]) {
				vertices[i].uv[0] = mesh->mTextureCoords[0][i].x;
				vertices[i].uv[1] = mesh->mTextureCoords[0][i].y;
			}
			else {
				vertices[i].uv[0] = vertices[i].uv[1] = 0.0f;
			}
		}

		// 2) Build index buffer (assume triangles)
		std::vector<uint32_t> indices;
		indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			const aiFace& face = mesh->mFaces[f];
			if (face.mNumIndices == 3) {
				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}
			// skip non-triangular faces
		}

		std::vector<uint32_t> remap1(vertices.size());
		size_t unique1 = meshopt_generateVertexRemap(
			remap1.data(),
			indices.data(), indices.size(),
			vertices.data(), vertices.size(), sizeof(VertexOpt)
		);

		std::vector<VertexOpt> weldedVerts(unique1);
		std::vector<uint32_t>  weldedIdx(indices.size());

		meshopt_remapVertexBuffer(
			weldedVerts.data(),
			vertices.data(), vertices.size(), sizeof(VertexOpt),
			remap1.data()
		);
		meshopt_remapIndexBuffer(
			weldedIdx.data(),
			indices.data(), indices.size(),
			remap1.data()
		);

		// 2) Simplify (this will rewrite weldedIdx in-place and return new index count)
		size_t targetIdx = weldedIdx.size() / 10;     // e.g. half the triangles
		float  maxError = 1.5f;                    // world-space error
		size_t simplifiedIdxCount = meshopt_simplify(
			weldedIdx.data(), weldedIdx.data(), weldedIdx.size(),
			reinterpret_cast<const float*>(weldedVerts.data()),
			weldedVerts.size(), sizeof(VertexOpt),
			targetIdx, maxError,
			meshopt_SimplifyLockBorder
		);

		// shrink index buffer to actual size
		weldedIdx.resize(simplifiedIdxCount);
		// 3) Second weld (throw away verts not referenced by any triangle)
		std::vector<uint32_t> remap2(weldedVerts.size());
		size_t unique2 = meshopt_generateVertexRemap(
			remap2.data(),
			weldedIdx.data(), weldedIdx.size(),
			weldedVerts.data(), weldedVerts.size(), sizeof(VertexOpt)
		);

		std::vector<VertexOpt> finalVerts(unique2);
		meshopt_remapVertexBuffer(
			finalVerts.data(),
			weldedVerts.data(), weldedVerts.size(), sizeof(VertexOpt),
			remap2.data()
		);
		meshopt_remapIndexBuffer(
			weldedIdx.data(),
			weldedIdx.data(), weldedIdx.size(),
			remap2.data()
		);

		// 4) GPU-friendly reordering (cache, overdraw, fetch)
		meshopt_optimizeVertexCache(
			weldedIdx.data(), weldedIdx.data(), weldedIdx.size(), finalVerts.size()
		);
		meshopt_optimizeOverdraw(
			weldedIdx.data(), weldedIdx.data(), weldedIdx.size(),
			reinterpret_cast<const float*>(finalVerts.data()),
			finalVerts.size(), sizeof(VertexOpt),
			1.05f
		);
		meshopt_optimizeVertexFetch(
			finalVerts.data(), weldedIdx.data(), weldedIdx.size(),
			finalVerts.data(), finalVerts.size(), sizeof(VertexOpt)
		);

		
		// 5) Write back to aiMesh (replace vertex & face arrays)
		// Free old data
		delete[] mesh->mVertices;
		delete[] mesh->mNormals;
		delete[] mesh->mTextureCoords[0];

		// Allocate new arrays
		mesh->mNumVertices = static_cast<unsigned int>(finalVerts.size());
		mesh->mVertices = new aiVector3D[mesh->mNumVertices];
		mesh->mNormals = new aiVector3D[mesh->mNumVertices];
		mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			mesh->mVertices[i] = aiVector3D(
				finalVerts[i].pos[0], finalVerts[i].pos[1], finalVerts[i].pos[2]
			);
			mesh->mNormals[i] = aiVector3D(
				finalVerts[i].normal[0], finalVerts[i].normal[1], finalVerts[i].normal[2]
			);
			mesh->mTextureCoords[0][i] = aiVector3D(
				finalVerts[i].uv[0], finalVerts[i].uv[1], 0.0f
			);
		}

		// Faces
		size_t newFaceCount = weldedIdx.size() / 3;
		delete[] mesh->mFaces;
		mesh->mNumFaces = static_cast<unsigned int>(newFaceCount);
		mesh->mFaces = new aiFace[mesh->mNumFaces];

		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			aiFace& face = mesh->mFaces[f];
			face.mNumIndices = 3;
			face.mIndices = new unsigned int[3];
			face.mIndices[0] = weldedIdx[f * 3 + 0];
			face.mIndices[1] = weldedIdx[f * 3 + 1];
			face.mIndices[2] = weldedIdx[f * 3 + 2];
		}
	}
}


// Build the actual geometry (vertices and indices) for a mesh from the FaceRect list and a given texture atlas configuration.
static void BuildMeshFromFaces(
	const std::vector<FaceRect>& faces,
	int texWidth, int texHeight,
	bool flatShading,
	const std::vector<color>& palette,
	aiMesh* mesh,
	const bbox& box,
	const vox_mat3& rotation  = {},
	const vox_vec3& translation  = {} 
) {
	// 1) Build raw voxel-space verts & indices
	struct Vertex {
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
		uint32_t colorIndex;
	};
	std::vector<Vertex>       verts;
	std::vector<unsigned int> indices;
	verts.reserve(faces.size() * 4);
	indices.reserve(faces.size() * 6);

	// New key that includes pos, normal, uv, and colorIndex
	struct VertKey {
		int px_i, py_i, pz_i;
		int nx_i, ny_i, nz_i;
		int u_i, v_i;
		uint32_t colorIndex;
		bool operator==(VertKey const& o) const {
			return px_i == o.px_i && py_i == o.py_i && pz_i == o.pz_i
				&& nx_i == o.nx_i && ny_i == o.ny_i && nz_i == o.nz_i
				&& u_i == o.u_i && v_i == o.v_i
				&& colorIndex == o.colorIndex;
		}
	};
	struct VertKeyHash {
		size_t operator()(VertKey const& k) const noexcept {
			// combine all 8 ints + colorIndex
			size_t h = 146527; // random seed
			auto mix = [&](size_t v) { h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };
			mix(std::hash<int>()(k.px_i));
			mix(std::hash<int>()(k.py_i));
			mix(std::hash<int>()(k.pz_i));
			mix(std::hash<int>()(k.nx_i));
			mix(std::hash<int>()(k.ny_i));
			mix(std::hash<int>()(k.nz_i));
			mix(std::hash<int>()(k.u_i));
			mix(std::hash<int>()(k.v_i));
			mix(std::hash<uint32_t>()(k.colorIndex));
			return h;
		}
	};

	std::unordered_map<VertKey, unsigned int, VertKeyHash> vertMap;
	vertMap.reserve(faces.size() * 4);

	auto addVertex = [&](float vx, float vy, float vz,
		float nx, float ny, float nz,
		float u, float v,
		uint32_t colorIndex) -> unsigned int
		{
			// quantize everything to integers so hashing is stable
			VertKey key;
			key.px_i = int(std::round(vx * 100.0f));
			key.py_i = int(std::round(vy * 100.0f));
			key.pz_i = int(std::round(vz * 100.0f));

			key.nx_i = int(std::round(nx * 1000.0f));
			key.ny_i = int(std::round(ny * 1000.0f));
			key.nz_i = int(std::round(nz * 1000.0f));

			key.u_i = int(std::round(u * texWidth * 100.0f));
			key.v_i = int(std::round(v * texHeight * 100.0f));

			key.colorIndex = colorIndex;

			// lookup/insert
			if (auto it = vertMap.find(key); it != vertMap.end()) {
				return it->second;
			}
			else {
				unsigned int idx = (unsigned int)verts.size();
				verts.push_back(Vertex{ vx,vy,vz, nx,ny,nz, u,v, colorIndex });
				vertMap[key] = idx;
				return idx;
			}
		};

	const float pixelW = 1.0f / float(texWidth),
		pixelH = 1.0f / float(texHeight),
		border = 1.0f;

	// winding‐flip test unchanged
	const float det =
		rotation.m00 * (rotation.m11 * rotation.m22 - rotation.m12 * rotation.m21)
		- rotation.m01 * (rotation.m10 * rotation.m22 - rotation.m12 * rotation.m20)
		+ rotation.m02 * (rotation.m10 * rotation.m21 - rotation.m11 * rotation.m20);
	const bool shouldInvert = (det < 0.0f);

	// 2) Emit all faces
	for (auto& face : faces) {
		// atlas UVs
		float u0 = (face.atlasX + border) * pixelW;
		float v0 = 1.0f - (face.atlasY + border) * pixelH;
		float u1 = (face.atlasX + border + face.w) * pixelW;
		float v1 = 1.0f - (face.atlasY + border + face.h) * pixelH;

		// face normal + 4 corners
		float nx = 0, ny = 0, nz = 0;
		float x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
		switch (face.orientation) {
		case 'X':
			nx = +1;
			x0 = face.constantCoord; y0 = face.vMin; z0 = face.uMin;
			x1 = face.constantCoord; y1 = face.vMax; z1 = face.uMin;
			x2 = face.constantCoord; y2 = face.vMax; z2 = face.uMax;
			x3 = face.constantCoord; y3 = face.vMin; z3 = face.uMax;
			break;
		case 'x':
			nx = -1;
			{
				float f = face.constantCoord, zmin = face.uMin, zmax = face.uMax;
				std::swap(zmin, zmax);
				x0 = f; y0 = face.vMin; z0 = zmin;
				x1 = f; y1 = face.vMax; z1 = zmin;
				x2 = f; y2 = face.vMax; z2 = zmax;
				x3 = f; y3 = face.vMin; z3 = zmax;
			}
			break;
		case 'Y':
			ny = +1;
			x0 = face.uMin; y0 = face.constantCoord; z0 = face.vMin;
			x1 = face.uMin; y1 = face.constantCoord; z1 = face.vMax;
			x2 = face.uMax; y2 = face.constantCoord; z2 = face.vMax;
			x3 = face.uMax; y3 = face.constantCoord; z3 = face.vMin;
			break;
		case 'y':
			ny = -1;
			{
				float f = face.constantCoord, zmin = face.vMin, zmax = face.vMax;
				std::swap(zmin, zmax);
				x0 = face.uMin; y0 = f; z0 = zmin;
				x1 = face.uMin; y1 = f; z1 = zmax;
				x2 = face.uMax; y2 = f; z2 = zmax;
				x3 = face.uMax; y3 = f; z3 = zmin;
			}
			break;
		case 'Z':
			nz = +1;
			x0 = face.uMin; y0 = face.vMin; z0 = face.constantCoord;
			x1 = face.uMin; y1 = face.vMax; z1 = face.constantCoord;
			x2 = face.uMax; y2 = face.vMax; z2 = face.constantCoord;
			x3 = face.uMax; y3 = face.vMin; z3 = face.constantCoord;
			break;
		case 'z':
			nz = -1;
			{
				float f = face.constantCoord, xmin = face.uMin, xmax = face.uMax;
				std::swap(xmin, xmax);
				x0 = xmin; y0 = face.vMin; z0 = f;
				x1 = xmin; y1 = face.vMax; z1 = f;
				x2 = xmax; y2 = face.vMax; z2 = f;
				x3 = xmax; y3 = face.vMin; z3 = f;
			}
			break;
		default:
			continue;
		}

		// add (and weld) the 4 verts
		auto i0 = addVertex(x0, y0, z0, nx, ny, nz, u0, v0, face.colorIndex);
		auto i1 = addVertex(x1, y1, z1, nx, ny, nz, u0, v1, face.colorIndex);
		auto i2 = addVertex(x2, y2, z2, nx, ny, nz, u1, v1, face.colorIndex);
		auto i3 = addVertex(x3, y3, z3, nx, ny, nz, u1, v0, face.colorIndex);

		// winding test
		vox_vec3 P0{ x0,y0,z0 }, P1{ x1,y1,z1 }, P2{ x2,y2,z2 };
		vox_vec3 triN = cross(P1 - P0, P2 - P0);
		bool baseIsCCW = (dot(triN, vox_vec3{ nx,ny,nz }) > 0.0f);
		bool finalIsCCW = shouldInvert ? !baseIsCCW : baseIsCCW;

		if (finalIsCCW) {
			indices.insert(indices.end(), { i0,i1,i2,  i0,i2,i3 });
		}
		else {
			indices.insert(indices.end(), { i0,i2,i1,  i0,i3,i2 });
		}
	}

	// 3) If smooth shading, normalize summed normals
	if (!flatShading) {
		for (auto& v : verts) {
			float L = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
			if (L > 0) { v.nx /= L; v.ny /= L; v.nz /= L; }
		}
	}

	// 4) Upload into aiMesh (unchanged)
	mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	mesh->mNumVertices = (unsigned int)verts.size();
	mesh->mVertices = new aiVector3D[verts.size()];
	mesh->mNormals = new aiVector3D[verts.size()];
	mesh->mTextureCoords[0] = new aiVector3D[verts.size()];
	mesh->mNumUVComponents[0] = 2;

	// compute pivot...
	vox_vec3 pivot = {
		(box.minX + box.maxX) * 0.5f,
		(box.minY + box.maxY) * 0.5f,
		(box.minZ + box.maxZ) * 0.5f
	};

	for (unsigned int i = 0; i < verts.size(); ++i) {
		// recenter → rotate → swizzle → translate → mirror (exactly as before)
		float x = verts[i].px - pivot.x,
			y = verts[i].py - pivot.y,
			z = verts[i].pz - pivot.z;
		float nx = verts[i].nx,
			ny = verts[i].ny,
			nz = verts[i].nz;

		// apply rotation to pos & normal...
		float tx = rotation.m00 * x + rotation.m01 * y + rotation.m02 * z;
		float ty = rotation.m10 * x + rotation.m11 * y + rotation.m12 * z;
		float tz = rotation.m20 * x + rotation.m21 * y + rotation.m22 * z;
		x = tx; y = ty; z = tz;
		tx = rotation.m00 * nx + rotation.m01 * ny + rotation.m02 * nz;
		ty = rotation.m10 * nx + rotation.m11 * ny + rotation.m12 * nz;
		tz = rotation.m20 * nx + rotation.m21 * ny + rotation.m22 * nz;
		nx = tx; ny = ty; nz = tz;

		// swizzle into Assimp
		aiVector3D pos{ x, z, y };
		aiVector3D norm{ nx, nz, ny };

		// translation w/ Y⇄Z swap, then un-mirror X
		pos.x += translation.x; pos.y += translation.z; pos.z += translation.y;
		pos.x = -pos.x;  norm.x = -norm.x;

		mesh->mVertices[i] = pos;
		mesh->mNormals[i] = norm;
		mesh->mTextureCoords[0][i] = aiVector3D(verts[i].u, verts[i].v, 0.f);
	}

	// 5) Build faces
	mesh->mNumFaces = (unsigned int)(indices.size() / 3);
	mesh->mFaces = new aiFace[mesh->mNumFaces];
	for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
		aiFace& faceOut = mesh->mFaces[f];
		faceOut.mNumIndices = 3;
		faceOut.mIndices = new unsigned int[3] {
			indices[3 * f + 0],
				indices[3 * f + 1],
				indices[3 * f + 2]
			};
	}
}

// Create and save a PNG texture from the atlas data
static bool SaveAtlasImage(const std::string& filename, int width, int height, const std::vector<unsigned char>& rgbaData)
{
	// Use stb_image_write to write PNG
	if (stbi_write_png(filename.c_str(), width, height, 4, rgbaData.data(), width * 4) == 0) {
		return false;
	}
	return true;
}

// Generate the texture atlas image data given the list of faces and palette colors
static void GenerateAtlasImage(
	int texWidth,
	int texHeight,
	const std::vector<FaceRect>& faces,
	const std::vector<vox_model>& models,
	const std::vector<color>& palette,
	std::vector<unsigned char>& outImage)
{
	const int border = 1;
	outImage.assign(texWidth * texHeight * 4, 0);

	// Helper to fetch RGBA from palette given a MagicaVoxel colorIndex (1–255)
	auto getRGBA = [&](uint8_t ci) {
		size_t idx = ci > 0 ? ci - 1 : 0;
		if (idx >= palette.size()) idx = palette.size() - 1;
		return palette[idx];
		};

	// Sample the colorIndex at a given (x,y,z)
	auto sampleCI = [&](int x, int y, int z, s32 modelIndex)->uint8_t {
		int cell = models[modelIndex].voxel_3dGrid[z][y][x];
		if (cell < 0) return 0;
		return models[modelIndex].voxels[cell].colorIndex;
		};

	for (auto& face : faces) {
		int x0 = face.atlasX;
		int y0 = face.atlasY;
		int w = face.w;
		int h = face.h;

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

				uint8_t ci = sampleCI(vx, vy, vz, face.modelIndex);
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
			int dstTop = ((y0 + 0) * texWidth + (x0 + border + ix)) * 4;
			memcpy(&outImage[dstTop], &outImage[srcTop], 4);

			// copy from last interior row → bottom border
			int srcBot = ((y0 + border + h - 1) * texWidth + (x0 + border + ix)) * 4;
			int dstBot = ((y0 + border + h) * texWidth + (x0 + border + ix)) * 4;
			memcpy(&outImage[dstBot], &outImage[srcBot], 4);
		}

		// Left & right
		for (int iy = 0; iy < h; ++iy) {
			// copy from first interior column → left border
			int srcL = ((y0 + border + iy) * texWidth + (x0 + border + 0)) * 4;
			int dstL = ((y0 + border + iy) * texWidth + (x0 + 0)) * 4;
			memcpy(&outImage[dstL], &outImage[srcL], 4);

			// copy from last interior column → right border
			int srcR = ((y0 + border + iy) * texWidth + (x0 + border + w - 1)) * 4;
			int dstR = ((y0 + border + iy) * texWidth + (x0 + border + w)) * 4;
			memcpy(&outImage[dstR], &outImage[srcR], 4);
		}

		// Finally fill corners by copying the appropriate interior pixel:
		// top-left corner
		memcpy(
			&outImage[((y0 + 0) * texWidth + (x0 + 0)) * 4],
			&outImage[((y0 + border) * texWidth + (x0 + border)) * 4],
			4
		);
		// top-right
		memcpy(
			&outImage[((y0 + 0) * texWidth + (x0 + border + w)) * 4],
			&outImage[((y0 + border) * texWidth + (x0 + border + w - 1)) * 4],
			4
		);
		// bottom-left
		memcpy(
			&outImage[((y0 + border + h) * texWidth + (x0 + 0)) * 4],
			&outImage[((y0 + border + h - 1) * texWidth + (x0 + border)) * 4],
			4
		);
		// bottom-right
		memcpy(
			&outImage[((y0 + border + h) * texWidth + (x0 + border + w)) * 4],
			&outImage[((y0 + border + h - 1) * texWidth + (x0 + border + w - 1)) * 4],
			4
		);
	}
}

static bool WriteSceneToFile(const std::vector<aiScene*> scenes, const std::string& outPath, const ExportOptions& options)
{

	// Determine export format from extension


	std::string ext = "";

	switch (options.OutputFormat)
	{
	case ModelFormat::FBX:
		ext = "fbx";
		break;

	case ModelFormat::OBJ:
		ext = "obj";
		break;
	default:
		LOG_ERROR("Format not implemented in writeToFile switch.");
		break;
	}


	Assimp::Exporter exporter;
	std::string formatId;
	const aiExportFormatDesc* selectedFormat = nullptr;
	for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i) {
		const aiExportFormatDesc* fmt = exporter.GetExportFormatDescription(i);
		if (fmt && fmt->fileExtension == ext) {
			selectedFormat = fmt;
			break;
		}
	}
	if (!selectedFormat)
	{
		LOG_ERROR("Unsupported export format: {0}", ext);
		return false;
	}
	formatId = selectedFormat->id;

	u32 preprocess = 0;

	aiMatrix4x4 scaleMat;
	aiMatrix4x4::Scaling(aiVector3D(options.Converting.Scale.x, options.Converting.Scale.y, options.Converting.Scale.z), scaleMat);

	size_t dot = outPath.find_last_of('.');

	for (size_t i = 0; i < scenes.size(); i++)
	{
		auto scene = scenes[i];

		OptimizeAssimpScene(scene);

		if (options.Converting.Meshing.WeldVertices)
		{
			preprocess = aiProcess_JoinIdenticalVertices;
			// for (size_t i = 0; i < scene->mNumMeshes; i++)
			// {
			// 	MergeIdenticalVertices(scene->mMeshes[i]);
			// }
		}

		const std::string convertedOutName = outPath.substr(0, dot) + (scenes.size() > 1 ? "_" + std::to_string(i) : "");
		scene->mRootNode->mTransformation = scaleMat * scene->mRootNode->mTransformation;
		aiReturn ret = exporter.Export(scene, formatId.c_str(), convertedOutName + "." + ext, preprocess);
		if (ret != aiReturn_SUCCESS)
		{
			std::cerr << "Export failed: " << exporter.GetErrorString() << "\n";
			return false;
		}

	}

	return true;

}

bbox ComputeMeshBoundingBox(const aiMesh* mesh) {
	bbox boundingBox = {
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()
	};
	if (!mesh || mesh->mNumVertices == 0)
		return boundingBox;

	for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
		const aiVector3D& v = mesh->mVertices[i];
		const aiVector3D& n = mesh->mNormals[i];
		boundingBox.minX = std::min(boundingBox.minX, v.x);
		boundingBox.minY = std::min(boundingBox.minY, v.y);
		boundingBox.minZ = std::min(boundingBox.minZ, v.z);

		boundingBox.maxX = std::max(boundingBox.maxX, v.x);
		boundingBox.maxY = std::max(boundingBox.maxY, v.y);
		boundingBox.maxZ = std::max(boundingBox.maxZ, v.z);
	}
	return boundingBox;
}

vox_vec3 TransformToMeshSpace(const vox_vec3& p,
	const vox_vec3& voxCenter,
	const vox_mat3& rot,
	const vox_vec3& trans)
{
	// 1. Translate so pivot is at origin (pivot recentering)
	vox_vec3 v = p - voxCenter;

	// 2. Apply MagicaVoxel's rotation
	v = rot * v;

	// 3. Swizzle axes to (x, z, y) to match Assimp's coordinate system
	vox_vec3 sw{ v.x, v.z, v.y };

	// 4. Apply translation (note Y↔Z swap)
	sw.x += trans.x;
	sw.y += trans.z;
	sw.z += trans.y;

	// 5. Mirror the X-axis to restore right-handedness
	sw.x = -sw.x;

	return sw;
}

aiMatrix4x4 BuildAiTransformMatrix(
	const vox_mat3& rot,      // 3×3 rotation from MagicaVoxel
	const vox_vec3& trans      // translation from MagicaVoxel (_t), in voxel units
) {
	// 1. Assemble a 4×4 matrix from rot and trans,
	//    with axis swap and X mirroring baked in.

	// Prepare rotation rows
	// MagicaVoxel uses row-vectors; we map to column-major aiMatrix4x4
	float R[4][4] = {
		{ rot.m00, rot.m10, rot.m20, 0.0f },
		{ rot.m01, rot.m11, rot.m21, 0.0f },
		{ rot.m02, rot.m12, rot.m22, 0.0f },
		{    0.0f,    0.0f,    0.0f, 1.0f }
	};

	// 2. Swap axes: convert (x, y, z) → (x, z, y)
	//    Effectively multiply on the right with a permutation matrix.
	aiMatrix4x4 perm;
	perm.a1 = 1.0f; // X → X
	perm.b3 = 1.0f; // Y → Z
	perm.c2 = 1.0f; // Z → Y

	// 3. Mirror X: scale X by –1
	aiMatrix4x4 mirror;
	mirror.a1 = -1.0f;

	// Combine rotation, permutation, and mirroring
	aiMatrix4x4 mRot(
		R[0][0], R[0][1], R[0][2], 0.0f,
		R[1][0], R[1][1], R[1][2], 0.0f,
		R[2][0], R[2][1], R[2][2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	aiMatrix4x4 xf = mirror * perm * mRot;

	// 4. Apply translation: note the Y ↔ Z swap
	aiVector3D t;
	t.x = trans.x;
	t.y = trans.z;
	t.z = trans.y;

	aiMatrix4x4::Translation(t, xf);
	return xf;
}

inline vox_vec3 Rotate(const vox_mat3& m, const vox_vec3& v)
{
	return
	{
	  m.m00 * v.x + m.m10 * v.y + m.m20 * v.z,
	  m.m01 * v.x + m.m11 * v.y + m.m21 * v.z,
	  m.m02 * v.x + m.m12 * v.y + m.m22 * v.z
	};
}


template<typename T>
std::shared_ptr<TextureData> GetTexture(T& faces, const std::vector<color>& palette,
	const std::vector<vox_model>& models, const bool texturesPOT)
{
	int atlasDim = 16;
	int usedW = 0;
	int usedH = 0;

	if (texturesPOT)
	{
		// Start from 16 and double
		while (true)
		{
			if (PackFacesIntoAtlas(atlasDim, faces))
			{
				break;
			}
			atlasDim *= 2;
			if (atlasDim > 4096)
			{ // safety break
				LOG_ERROR("Could not pack texture atlas up to 4096 for frame ");// << frameIndex << "\n";
				break;
			}
		}

		usedW = atlasDim;
		usedH = atlasDim;
	}
	else
	{
		// Start with 16 and grow by 16 steps or double as needed (non-POT allowed)
		while (true)
		{
			if (PackFacesIntoAtlas(atlasDim, faces))
			{
				break;
			}

			atlasDim += 16;

			if (atlasDim > 4096)
			{
				LOG_ERROR("Could not pack texture atlas up to 4096 for frame ");
				break;
			}
		}
		// Shrink to actual used size
		// We can compute used width and height from faces placement
		for (auto& fr : faces)
		{
			usedW = std::max(usedW, fr.atlasX + fr.w + 2);
			usedH = std::max(usedH, fr.atlasY + fr.h + 2);
		}

		atlasDim = std::max(usedW, usedH);
	}
	// Create image
	auto textureData = std::make_shared<TextureData>();
	textureData->Width = usedW;
	textureData->Height = usedH;

	LOG_INFO("Texture size: ({0}, {1})", textureData->Width, textureData->Height);

	GenerateAtlasImage(atlasDim, atlasDim, faces, models, palette, textureData->Buffer);

	return textureData;
}

static std::shared_ptr<TextureData> BuildImage(bool generateTextures, bool texturesPOT, std::vector<FaceRect>& faces, 
											   const std::vector<color>& palette, const std::vector<vox_model>& models)
{

	s32 texWidth = 16, texHeight = 16;
	if (generateTextures)
	{
		// TODO: to create an atlas of all the meshes, the property colorIndex, in face should be updated.
		const auto textureData = GetTexture(faces, palette, models, texturesPOT);


		texWidth = textureData->Width;
		texHeight = textureData->Height;

		return textureData;
	}

	return nullptr;
}

// TODO: start simple, from the begining, the whole code base has a problem of code duplication.
static std::vector<aiScene*> GetModels(const vox_file* voxData, const s32 frameIndex, const std::string& outputPath, const ConvertOptions& options)
{

	struct MeshWrapData
	{
		aiMesh* Mesh;
		std::string imageName;
	};


	std::vector<MeshWrapData> meshes;
	std::vector<aiNode*> shapeNodes;

	s32 materialIndex = 0;

	const std::vector<vox_vec3>& pivots = options.Pivots;// Rotate(wxf.rot, options.Pivot);

	struct ModelData 
	{
		std::vector<FaceRect> faces;
	};

	if (voxData->shapes.size() > 0)
	{
		const bool canIteratePivots = pivots.size() > 1 && pivots.size() == voxData->shapes.size();

		if (pivots.size() == 0)
		{
			LOG_WARN("Default pivot '(0.5, 0.5, 0.5)' will be used for all meshes.");
		}
		else if (pivots.size() == 1)
		{
			LOG_WARN("First pivot will be used for all meshes.");
		}
		else if (!canIteratePivots)
		{
			LOG_WARN("Pivots '{0}' do not match the amount of meshes: {1}. Using default (0.5, 0.5, 0.5)", pivots.size(), voxData->shapes.size());
		}
		else
		{
			LOG_INFO("Custom pivots in use: '{0}'", pivots.size());
		}

		std::vector<color> pallete = voxData->palette;
		std::shared_ptr<TextureData> textureData = nullptr;
		std::string imageName = "";
		std::string baseName = outputPath;
		const size_t dot = outputPath.find_last_of('.');
		std::vector<FaceRect> faces;

		std::vector<FaceRect> mergedFaces = {};
		
		std::unordered_map<s32, std::vector<FaceRect>> modelsData = {};

		if (dot != std::string::npos)
		{
			baseName = outputPath.substr(0, outputPath.find_last_of('.'));
		}

if (!options.Texturing.SeparateTexturesPerMesh)
			{
		for (auto& shpKV : voxData->shapes)
		{
			const vox_nSHP& shape = shpKV.second;

			int modelId = -1;

			if (shape.models.size() == 1)
			{
				modelId = shape.models[0].modelID;
			}
			else
			{
				for (const auto& m : shape.models)
				{
					if (m.frameIndex == frameIndex)
					{
						modelId = m.modelID;
						break;
					}
				}
			}

			if (modelId < 0)
			{
				continue;
			}

			faces = GreedyMesh_Atlas(voxData->voxModels[modelId], voxData->sizes[modelId], modelId);

			// --- Below

			
				mergedFaces.insert(mergedFaces.end(), faces.begin(), faces.end());
			}
			textureData = BuildImage(options.Texturing.GenerateTextures, options.Texturing.TexturesPOT, mergedFaces, voxData->palette, voxData->voxModels);

			LOG_INFO("Atlas export: {0}", baseName + "_atlas.png");

			imageName = baseName + "_atlas.png";
			SaveAtlasImage(imageName, textureData->Width, textureData->Height, textureData->Buffer);

			// Very slow
			for (size_t i = 0; i < mergedFaces.size(); i++)
			{
				modelsData[mergedFaces[i].modelIndex].push_back(mergedFaces[i]);
			}
		}

		s32 shapeIndex{};
		for (auto& shpKV : voxData->shapes)
		{
			vox_vec3 currentPivot{ 0.5f, 0.5f, 0.5f };

			if (canIteratePivots)
			{
				currentPivot = pivots[shapeIndex];
			}
			else if (pivots.size() == 1)
			{
				currentPivot = pivots[0];
			}

			std::string name = shpKV.second.attributes.count("_name") ? shpKV.second.attributes.at("_name") : "vox";

			const vox_nSHP& shape = shpKV.second;

			int modelId = -1;

			if (shape.models.size() == 1)
			{
				modelId = shape.models[0].modelID;
			}
			else
			{
				for (const auto& m : shape.models)
				{
					if (m.frameIndex == frameIndex)
					{
						modelId = m.modelID;
						break;
					}
				}
			}

			if (modelId < 0)
			{
				continue;
			}

			// TODO Move this to another function so it can be reused for everything else--------------------------------------------------------------------------------------------------

			// Generate mesh for this shape/model

			if (options.Texturing.SeparateTexturesPerMesh)
			{
				faces = GreedyMesh_Atlas(voxData->voxModels[modelId], voxData->sizes[modelId], modelId);

				textureData = BuildImage(options.Texturing.GenerateTextures, options.Texturing.TexturesPOT, faces, voxData->palette, voxData->voxModels);

				// Remove this from here
				if (options.Texturing.GenerateTextures)
				{


					imageName = baseName + "_frame" + std::to_string(shapeIndex++) + ".png";
					SaveAtlasImage(imageName, textureData->Width, textureData->Height, textureData->Buffer);
				}
			}
			else 
			{
				faces = modelsData[modelId];
			}


			aiMesh* mesh = new aiMesh();

			int shapeNodeId = shape.nodeID;
			const vox_nTRN* trnNode = nullptr;
			for (const auto& trnKV : voxData->transforms)
			{
				if (trnKV.second.childNodeID == shapeNodeId)
				{
					trnNode = &trnKV.second;
					break;
				}
			}

			// TODO: This bounding box seems off
			auto box = voxData->voxModels[modelId].boundingBox;

			// If you want to support groups (nGRP), you may have to walk up to the root and find the chain.
			vox_transform wxf = AccumulateWorldTransform(shape.nodeID, frameIndex, *voxData);

			// Build mesh and apply MagicaVoxel rotation+translation directly into vertices:
			BuildMeshFromFaces(
				faces,
				textureData->Width, textureData->Height,
				options.Meshing.FlatShading,
				pallete,
				mesh,
				box,          // pivot centering
				wxf.rot,     // MagicaVoxel 3×3 rotation
				wxf.trans    // MagicaVoxel translation
			);

			// 3) Recenter every vertex so that the mesh’s center is at the origin


			// Assign material index...

			if (options.Meshing.GenerateMaterials)
			{
				mesh->mMaterialIndex = options.Meshing.MaterialPerMesh && !options.ExportMeshesSeparatelly ? materialIndex++ : 0;
			}
			else
			{
				mesh->mMaterialIndex = 0;
			}
			// Record mesh index and create node with identity transform:
			unsigned int meshIndex = static_cast<unsigned int>(meshes.size());

			aiNode* node = new aiNode();
			node->mName = aiString(name);
			node->mNumMeshes = 1;

			if (options.ExportMeshesSeparatelly)
			{
				node->mMeshes = new unsigned int[1] { 0 };
			}
			else
			{
				node->mMeshes = new unsigned int[1] { meshIndex };
			}


			// box = TransformAABB(box, wxf.rot, wxf.trans);
			//  float cxx = box.minX + (box.maxX - box.minX) * options.Pivot.x;
			//  float cyy = box.minY + (box.maxY - box.minY) * options.Pivot.y;
			//  float czz = box.minZ + (box.maxZ - box.minZ) * options.Pivot.z;

			auto bbbox = ComputeMeshBoundingBox(mesh);
			float cxx = bbbox.minX + (bbbox.maxX - bbbox.minX) * currentPivot.x;
			float cyy = bbbox.minY + (bbbox.maxY - bbbox.minY) * currentPivot.y;
			float czz = bbbox.minZ + (bbbox.maxZ - bbbox.minZ) * currentPivot.z;

			aiVector3D cent(cxx, cyy, czz);

			if (options.Meshing.MeshesToWorldCenter)
			{
				for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
				{
					mesh->mVertices[i] -= cent;
				}
			}
			else
			{
				for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
				{
					mesh->mVertices[i] -= cent;
				}

				aiMatrix4x4 C;
				aiMatrix4x4::Translation(cent, C);
				node->mTransformation = C * node->mTransformation;
			}


			// ----------------------------------------------------------------------------------------------------------------------------------------------------------

			meshes.push_back({ mesh, imageName });
			shapeNodes.push_back(node);
		}
	}
	else if (voxData->voxModels.size() > 0)
	{

	}
	else
	{
		LOG_WARN(".Vox File has no meshes");
	}

	std::vector<aiScene*> scenes{};

	if (options.ExportMeshesSeparatelly)
	{
		for (size_t i = 0; i < meshes.size(); i++)
		{
			aiScene* sceneSplit = new aiScene();

			// — create & initialize the root node —
			sceneSplit->mRootNode = new aiNode();
			sceneSplit->mRootNode->mName = aiString("RootNode");
			sceneSplit->mRootNode->mTransformation = aiMatrix4x4();   // identity
			// ** zero out the root node’s mesh list **
			sceneSplit->mRootNode->mNumMeshes = 0;
			sceneSplit->mRootNode->mMeshes = nullptr;

			// now attach exactly one child node:
			sceneSplit->mRootNode->mNumChildren = 1;
			sceneSplit->mRootNode->mChildren = new aiNode * [1] { shapeNodes[i] };
			sceneSplit->mRootNode->mChildren[0]->mMeshes[0] = 0; // Overwrite to set the index to 0



			// — populate the scene’s mesh and material arrays —
			sceneSplit->mNumMeshes = 1;
			sceneSplit->mMeshes = new aiMesh * [1] { meshes[i].Mesh };

			if (options.Meshing.GenerateMaterials)
			{
				aiString texPath(meshes[i].imageName);
				aiMaterial* mat = new aiMaterial();
				mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

				// Since the meshes will be exported separatelly, always create a material per mesh
				sceneSplit->mNumMaterials = 1;
				sceneSplit->mMaterials = new aiMaterial * [1] { mat };

			}
			else
			{
				sceneSplit->mNumMaterials = 0;
				sceneSplit->mMaterials = nullptr;
			}
			scenes.push_back(sceneSplit);
		}
	}
	else
	{
		aiScene* scene = new aiScene();
		scene->mRootNode = new aiNode();
		scene->mNumMeshes = meshes.size();
		scene->mMeshes = new aiMesh * [meshes.size()];

		// Attach all shape nodes to root
		scene->mRootNode->mNumChildren = shapeNodes.size();
		scene->mRootNode->mChildren = new aiNode * [shapeNodes.size()];

		// TODO: set one material per mesh, or share a material? for shared materials, the texture individial export option should be turned off, since the material needs the whole atlas.

		if (options.Meshing.GenerateMaterials)
		{
			if (options.Meshing.MaterialPerMesh)
			{
				scene->mNumMaterials = meshes.size();
				scene->mMaterials = new aiMaterial * [meshes.size()];
			}
			else
			{
				LOG_ERROR("IMPLEMENT shared materials");
				throw;
				scene->mNumMaterials = 1;
				scene->mMaterials = new aiMaterial * [1];

			}
		}
		else
		{
			scene->mNumMaterials = 0;
			scene->mMaterials = nullptr;
		}

		for (size_t i = 0; i < meshes.size(); ++i)
		{
			scene->mMeshes[i] = meshes[i].Mesh;

			s32 matIndex = meshes[i].Mesh->mMaterialIndex;

			if (options.Meshing.GenerateMaterials)
			{
				aiString texPath(meshes[i].imageName);
				aiMaterial* mat = new aiMaterial();
				mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
				scene->mMaterials[matIndex] = mat;
			}
		}

		// Set all the nodes to the root.
		for (size_t i = 0; i < scene->mRootNode->mNumChildren; ++i)
		{
			scene->mRootNode->mChildren[i] = shapeNodes[i];
		}

		scenes = { scene };
	}

	for (auto sceneitem : scenes)
	{
		// Adds default material, some formats require at least one.
		if (sceneitem->mNumMaterials == 0)
		{
			sceneitem->mNumMaterials = 1;
			sceneitem->mMaterials = new aiMaterial * [1] {new aiMaterial};
		}

		if (options.Meshing.RemoveTJunctions)
		{
			for (unsigned int i = 0; i < sceneitem->mNumMeshes; ++i)
			{
				CleanUpMesh(sceneitem->mMeshes[i]);
			}

			LOG_INFO("Done cleaning up meshes count: {0}", sceneitem->mNumMeshes);
		}
	}


	return scenes;
}



const std::vector<aiScene*> Run(const vox_file* voxData, const std::string& outputPath, const ConvertOptions& options)
{
	if (!voxData || !voxData->isValid)
	{
		std::cerr << "Failed to read voxel file or file is invalid.\n";
		return {};
	}

	// Determine if we have multiple frames (multiple models or transform frames)
	s32 frameCount = 0;

	// find any transform with framesCount > 1
	for (auto& kv : voxData->transforms)
	{
		frameCount = std::max(frameCount, kv.second.framesCount);
	}

	LOG_INFO("Version: {0}", voxData->header.version);
	LOG_INFO("Transforms: {0}", voxData->transforms.size());
	LOG_INFO("Models: {0}", voxData->voxModels.size());
	LOG_INFO("Shapes: {0}", voxData->shapes.size());
	LOG_INFO("FrameCount: {0}", frameCount);

	if (frameCount == 0 && voxData->voxModels.size() == 0 && voxData->shapes.size() == 0)
	{
		std::cerr << "No voxel models in the file.\n";
		return {};
	}

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

	if (options.ExportFramesSeparatelly && frameCount >= 1 && voxData->shapes.size() > 0)
	{
		std::vector<aiScene*> scenesOut{};

		for (s32 fi = 0; fi < frameCount; ++fi)
		{
			// Prepare a new minimal scene for this frame
			LOG_INFO("Frame processing: {0}", fi);
			auto scenes = GetModels(voxData, fi, outputPath, options);

			for (size_t j = 0; j < scenes.size(); j++)
			{
				// Export this scene
				std::string frameOut = outputPath;
				// Insert frame number before extension
				if (dot != std::string::npos)
				{
					frameOut = outputPath.substr(0, outputPath.find_last_of('.')) + "_frame" + std::to_string(j) + outputPath.substr(outputPath.find_last_of('.'));
				}
				else
				{
					frameOut = outputPath + "_frame" + std::to_string(j);
				}

				scenes[j]->mName = frameOut;
			}

			scenesOut.insert(scenesOut.end(), std::make_move_iterator(scenes.begin()), std::make_move_iterator(scenes.end()));
		}

		return scenesOut;
	}
	else
	{
		frameCount = voxData->voxModels.size();
		// Combined scene (either single frame or multiple frames in one file)
		size_t meshCount = frameCount;
		scene->mMeshes = new aiMesh * [meshCount];
		scene->mMaterials = new aiMaterial * [meshCount];
		scene->mNumMeshes = (unsigned int)meshCount;
		scene->mNumMaterials = (unsigned int)(options.Texturing.SeparateTexturesPerMesh ? meshCount : 1);
		if (options.Texturing.SeparateTexturesPerMesh) {
			// each mesh gets its own material
			for (size_t i = 0; i < meshCount; ++i) {
				scene->mMaterials[i] = new aiMaterial();
			}
		}
		else {
			// one material for all meshes
			scene->mMaterials[0] = new aiMaterial();
			for (size_t i = 1; i < meshCount; ++i) {
				scene->mMaterials[i] = scene->mMaterials[0];
			}
		}
		// Root node children for each mesh
		scene->mRootNode->mNumChildren = (unsigned int)meshCount;
		scene->mRootNode->mChildren = new aiNode * [meshCount];
		// Iterate through frames
		int globalAtlasSize = 0;
		std::vector<unsigned char> globalImage;
		if (!options.Texturing.SeparateTexturesPerMesh)
		{
			// If one atlas for all, gather all faces first
			std::vector<FaceRect> allFaces;
			std::unordered_set<uint8_t> usedColors;
			for (size_t i = 0; i < meshCount; ++i) {
				size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1);
				std::vector<FaceRect> faces = GreedyMesh_Atlas(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);
				// Tag faces with an offset or id if needed (not needed for atlas, we just combine)
				allFaces.insert(allFaces.end(), faces.begin(), faces.end());
			}
			// Pack combined faces
			int dim = 16;
			if (options.Texturing.TexturesPOT) {
				while (true) {
					if (PackFacesIntoAtlas(dim, allFaces)) break;
					dim *= 2;
					if (dim > 8192) break;
				}
			}
			else {
				while (true) {
					if (PackFacesIntoAtlas(dim, allFaces)) break;
					dim += 16;
					if (dim > 8192) break;
				}
				// Shrink to used extents
				int usedW = 0, usedH = 0;
				for (auto& fr : allFaces) {
					usedW = std::max(usedW, fr.atlasX + fr.w + 2);
					usedH = std::max(usedH, fr.atlasY + fr.h + 2);
				}
				dim = std::max(usedW, usedH);
			}
			globalAtlasSize = dim;
			//GenerateAtlasImage(dim, dim, allFaces, voxData->palette, globalImage);
			GenerateAtlasImage(dim, dim, allFaces, voxData->voxModels, voxData->palette, globalImage);

			// Save atlas image
			std::string baseName = outputPath;
			if (dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
			std::string atlasName = baseName + "_atlas.png";
			SaveAtlasImage(atlasName, dim, dim, globalImage);

			LOG_INFO("Atlas saved");

			// Assign this texture to the single material
			aiString texPath(atlasName);
			scene->mMaterials[0]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
			// Now need to build each mesh's geometry from the portion of faces belonging to that mesh
			// We should separate faces by frame segment: because allFaces is combined list.
			// We can reconstruct segmentation because we processed each frame sequentially and appended.
			// So we can do another loop now generating geometry for each frame from allFaces:
			size_t faceOffset = 0;
			for (size_t i = 0; i < meshCount; ++i) {
				size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1);
				// Remesh the frame to get number of faces:            
				std::vector<FaceRect> frameFaces = GreedyMesh_Atlas(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);
				// Now copy that many faces from allFaces (they should correspond in order to this frame).
				std::vector<FaceRect> facesForMesh;
				facesForMesh.insert(facesForMesh.end(), allFaces.begin() + faceOffset, allFaces.begin() + faceOffset + frameFaces.size());
				faceOffset += frameFaces.size();
				// Create mesh
				aiMesh* mesh = new aiMesh();
				scene->mMeshes[i] = mesh;

				auto& sz = voxData->sizes[modelIndex];
				auto& mdl = voxData->voxModels[modelIndex];
				auto  box = mdl.boundingBox;

				BuildMeshFromFaces(facesForMesh, globalAtlasSize, globalAtlasSize, options.Meshing.FlatShading, voxData->palette, mesh, box);
				LOG_INFO("Build meshes from faces, mesh: {0}", i);

				// TODO: position origin issue, take into account the position of the objects, this should be used, reynardo
				//voxData->transforms.at(0).frameAttrib[0].translation;
				//--voxData->transforms.at(0).frameAttrib[0].rotation;
				//-----


				mesh->mMaterialIndex = options.Texturing.SeparateTexturesPerMesh ? (int)i : 0;
				// Create node for this mesh
				aiNode* node = new aiNode();
				node->mName = aiString("Frame" + std::to_string(i));
				node->mNumMeshes = 1;
				node->mMeshes = new unsigned int[1];
				node->mMeshes[0] = i;
				aiMatrix4x4 rot;
				aiMatrix4x4::RotationX(-static_cast<float>(AI_MATH_PI / 2.0f), rot);
				//node->mTransformation = rot;

				scene->mRootNode->mChildren[i] = node;

				LOG_INFO("Completed mesh: {0}", i);

			}
		}
		else
		{
			// separateTexturesPerMesh case:
			for (size_t i = 0; i < meshCount; ++i)
			{
				size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1);

				std::vector<FaceRect> faces = GreedyMesh_Atlas(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);
				int dim = 16;
				if (options.Texturing.TexturesPOT)
				{
					while (true)
					{
						if (PackFacesIntoAtlas(dim, faces)) break;
						dim *= 2;
						if (dim > 4096) break;
					}
				}
				else
				{
					while (true)
					{
						if (PackFacesIntoAtlas(dim, faces)) break;
						dim += 16;
						if (dim > 4096) break;
					}
					// shrink
					int usedW = 0, usedH = 0;
					for (auto& fr : faces) {
						usedW = std::max(usedW, fr.atlasX + fr.w + 2);
						usedH = std::max(usedH, fr.atlasY + fr.h + 2);
					}
					dim = std::max(usedW, usedH);
				}
				// Create atlas for this mesh
				std::vector<unsigned char> img;
				//GenerateAtlasImage(dim, dim, faces, voxData->palette, img);
				GenerateAtlasImage(dim, dim, faces, voxData->voxModels, voxData->palette, img);

				std::string baseName = outputPath;
				if (dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
				std::string imageName = baseName + "_mesh" + std::to_string(i) + ".png";
				SaveAtlasImage(imageName, dim, dim, img);

				aiString texPath(imageName);
				scene->mMaterials[i]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
				// Create mesh geometry
				aiMesh* mesh = new aiMesh();
				scene->mMeshes[i] = mesh;

				auto& sz = voxData->sizes[modelIndex];
				auto& mdl = voxData->voxModels[modelIndex];
				auto  box = mdl.boundingBox;

				BuildMeshFromFaces(faces, dim, dim, options.Meshing.FlatShading, voxData->palette, mesh, box);

				mesh->mMaterialIndex = (int)i;
				// Node
				aiNode* node = new aiNode();
				node->mName = aiString("Mesh" + std::to_string(i));
				node->mNumMeshes = 1;
				node->mMeshes = new unsigned int[1];
				node->mMeshes[0] = i;
				aiMatrix4x4 rot;
				aiMatrix4x4::RotationX(-static_cast<float>(AI_MATH_PI / 2.0f), rot);
				node->mTransformation = rot;

				scene->mRootNode->mChildren[i] = node;
			}
		}
		// If there was only one mesh in scene (no children used above), attach it directly to root node

		LOG_INFO("TJuntctions: {0}", options.Meshing.RemoveTJunctions);

		// TODO: This makes the algorithm freeze when a vox has multiple frames, and is exported as no separated
		if (options.Meshing.RemoveTJunctions)
		{
			for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
			{
				CleanUpMesh(scene->mMeshes[m]);
			}

			LOG_INFO("Done cleaning up meshes count: {0}", scene->mNumMeshes);
		}

		// Export combined scene
		// if(!exportScene(outputPath)) {
		//     std::cerr << "Failed to export scene.\n";
		//     return 1;
		// } else {
		//     std::cout << "Exported " << outputPath << " successfully.\n";
		// }

		return { scene };
	}

	// Clean up dynamically allocated scene data for combined case
	if (!options.ExportFramesSeparatelly) {
		// Clean materials (if unique)
		std::unordered_set<aiMaterial*> uniqueMats;
		for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
			uniqueMats.insert(scene->mMaterials[i]);
		}
		for (aiMaterial* mat : uniqueMats) {
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
	return {};
}



ExportResults GreedyMesher::ExportVoxToModel(const std::string& inVoxPath, const std::string& outExportPath, const ExportOptions& options)
{
	std::shared_ptr<vox_file> voxData = VoxParser::read_vox_file(inVoxPath.c_str());
	const auto scenes = Run(voxData.get(), outExportPath, options.Converting);

	ExportResults results{};

	if (scenes.size() > 0)
	{
		WriteSceneToFile(scenes, outExportPath, options);

		for (size_t i = 0; i < scenes.size(); i++)
		{
			delete scenes[i];
		}

		results.Convert.Msg = ConvertMSG::SUCESS;
	}
	else
	{
		results.Convert.Msg = ConvertMSG::FAILED;
	}

	return results;
}

ExportResults GreedyMesher::ExportVoxToModel(const char* buffer, int size, const ExportOptions& options)
{
	LOG_ERROR("Not implemented");
	throw;

	return {};
}

MemData GreedyMesher::VoxToMem(const std::string& inVoxPath, const ConvertOptions& options)
{
	LOG_ERROR("Not implemented");
	throw;

	return {};

}

MemData GreedyMesher::VoxToMem(const char* buffer, int size, const ConvertOptions& options)
{
	LOG_ERROR("Not implemented");
	throw;
	return {};
}

void GreedyMesher::ExportVoxToModelAsync(const char* buffer, int size, const ExportOptions& options, std::function<void(ExportResults)> callback)
{
	LOG_ERROR("Not implemented");
	throw;
}

void GreedyMesher::GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(MemData)> callback)
{
	LOG_ERROR("Not implemented");
	throw;
}
