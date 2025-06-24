#pragma once
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <meshoptimizer/src/meshoptimizer.h>
#include <stack>
#include <Unvoxeller/Log/Log.h>
#include <Unvoxeller/Data/UnvoxMesh.h>
#include <Unvoxeller/Data/UnvoxScene.h>


using namespace Unvoxeller;

struct MyTraits : public OpenMesh::DefaultTraits 
{
	// turn on per-vertex normals and 2D texcoords
	VertexAttributes(OpenMesh::Attributes::Normal |
		OpenMesh::Attributes::TexCoord2D);
};
using TriMesh = OpenMesh::TriMesh_ArrayKernelT<MyTraits>;
#include <Unvoxeller/TjunctionsFixer.h>


// Import an aiMesh into OpenMesh
static void convertAiMeshToOpenMesh(UnvoxMesh* aimesh, TriMesh& om) 
{
	om.clear();

	// Enable per‐vertex normals and UVs in OpenMesh
	om.request_vertex_normals();
	om.request_vertex_texcoords2D();

	// Keep a handle list for adding faces
	std::vector<TriMesh::VertexHandle> vhandle(aimesh->Vertices.size());

	// Import vertices, normals and UVs
	for (unsigned int i = 0; i < aimesh->Vertices.size(); ++i) 
	{
		// Position
		const auto& p = aimesh->Vertices[i];
		vhandle[i] = om.add_vertex({ p.x, p.y, p.z });

		// Normal (if present)
		if (aimesh->Normals.size() > 0) 
		{
			const auto& n = aimesh->Normals[i];
			om.set_normal(vhandle[i], { n.x, n.y, n.z });
		}

		// UV (if present)
		if (aimesh->UVs.size() > 0)
		{
			const auto& uv = aimesh->UVs[i];
			om.set_texcoord2D(vhandle[i], { uv.x, uv.y });
		}
	}

	// Import faces (triangles)
	for (unsigned int f = 0; f < aimesh->Faces.size(); ++f) 
	{
		const auto& face = aimesh->Faces[f];
		std::vector<TriMesh::VertexHandle> fv;
		fv.reserve(face.Indices.size());
		for (unsigned int j = 0; j < face.Indices.size(); ++j)
		{
			fv.push_back(vhandle[face.Indices[j]]);
		}
		om.add_face(fv);
	}

	// Optionally update OpenMesh’s internal normals if you want to recompute:
	// om.update_normals();
}

// Export OpenMesh back into the existing aiMesh (overwriting its arrays)
// Converts an OpenMesh TriMesh (with per‐vertex normals and 2D texcoords) back into an existing aiMesh
static void convertOpenMeshToAiMesh(TriMesh& om, UnvoxMesh* aimesh)
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

	aimesh->Vertices.resize(nv);
	aimesh->Normals.resize(nv);
	aimesh->UVs.resize(nv);

	// Map from OpenMesh vertex index → aiMesh index
	std::vector<unsigned int> idxMap(nv);
	unsigned int idx = 0;
	for (auto vh : om.vertices()) 
	{
		// Position
		auto  p = om.point(vh);
		aimesh->Vertices[idx] = { p[0], p[1], p[2] };

		// Normal
		auto  n = om.normal(vh);
		aimesh->Normals[idx] = { n[0], n[1], n[2] };

		// UV
		auto  uv = om.texcoord2D(vh);
		aimesh->UVs[idx] = { uv[0], uv[1] };

		idxMap[vh.idx()] = idx++;
	}

	// --- Faces (triangles) ---
	const unsigned int nf = static_cast<unsigned int>(om.n_faces());

	aimesh->Faces.resize(nf);

	idx = 0;
	for (auto fh : om.faces())
	{
		auto& af = aimesh->Faces[idx];
		af.Indices.resize(3);

		int vi = 0;
		for (auto fv_it = om.cfv_iter(fh); fv_it.is_valid(); ++fv_it)
		{
			af.Indices[vi++] = idxMap[fv_it->idx()];
		}
		++idx;
	}
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

static void CleanUpMesh(UnvoxMesh* mesh)
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


inline vox_vec3 crossProduct(const vox_vec3& a, const vox_vec3& b)
{
	return vox_vec3(
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
void OptimizeAssimpScene(UnvoxScene* scene) 
{
	LOG_INFO("Optimizing mesh");

	for (unsigned int mi = 0; mi < scene->Meshes.size(); ++mi)
	{
		UnvoxMesh* mesh = scene->Meshes[mi].get();

		// 1) Build interleaved vertex buffer
		std::vector<VertexOpt> vertices(mesh->Vertices.size());

		for (unsigned int i = 0; i < mesh->Vertices.size(); ++i) 
		{
			// Position
			vertices[i].pos[0] = mesh->Vertices[i].x;
			vertices[i].pos[1] = mesh->Vertices[i].y;
			vertices[i].pos[2] = mesh->Vertices[i].z;
			// Normal (if present)
			if (mesh->Normals.size() > 0) 
			{
				vertices[i].normal[0] = mesh->Normals[i].x;
				vertices[i].normal[1] = mesh->Normals[i].y;
				vertices[i].normal[2] = mesh->Normals[i].z;
			}
			else 
			{
				vertices[i].normal[0] = vertices[i].normal[1] = vertices[i].normal[2] = 0.0f;
			}
			// UV0 (if present)
			if (mesh->UVs.size() > i) 
			{
				vertices[i].uv[0] = mesh->UVs[i].x;
				vertices[i].uv[1] = mesh->UVs[i].y;
			}
			else
			{
				vertices[i].uv[0] = vertices[i].uv[1] = 0.0f;
			}
		}

		// 2) Build index buffer (assume triangles)
		std::vector<uint32_t> indices;
		indices.reserve(mesh->Faces.size() * 3);

		for (unsigned int f = 0; f < mesh->Faces.size(); ++f) 
		{
			const auto& face = mesh->Faces[f];

			if (face.Indices.size() == 3) 
			{
				indices.push_back(face.Indices[0]);
				indices.push_back(face.Indices[1]);
				indices.push_back(face.Indices[2]);
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

		
		// Allocate new arrays
		mesh->Vertices.resize(finalVerts.size());
		mesh->Normals.resize(mesh->Vertices.size());
		mesh->UVs.resize(mesh->Vertices.size());

		for (unsigned int i = 0; i < mesh->Vertices.size(); ++i) 
		{
			mesh->Vertices[i] = { finalVerts[i].pos[0], finalVerts[i].pos[1], finalVerts[i].pos[2] };

			mesh->Normals[i] = { finalVerts[i].normal[0], finalVerts[i].normal[1], finalVerts[i].normal[2] };
		
			mesh->UVs[i] = { finalVerts[i].uv[0], finalVerts[i].uv[1] };
		}

		// Faces
		size_t newFaceCount = weldedIdx.size() / 3;

		mesh->Faces.resize(newFaceCount);

		for (unsigned int f = 0; f < mesh->Faces.size(); ++f) 
		{
			auto& face = mesh->Faces[f];

			face.Indices.resize(3);
			face.Indices[0] = weldedIdx[f * 3 + 0];
			face.Indices[1] = weldedIdx[f * 3 + 1];
			face.Indices[2] = weldedIdx[f * 3 + 2];
		}
	}
}


