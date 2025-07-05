#include <Unvoxeller/MeshBuilder.h>

namespace Unvoxeller
{
    // Build the actual geometry (vertices and indices) for a mesh from the FaceRect list and a given texture atlas configuration.
 std::shared_ptr<UnvoxMesh> MeshBuilder::BuildMeshFromFaces(
        const std::vector<FaceRect>& faces,
        int texWidth, int texHeight,
        bool flatShading,
        const std::vector<color>& palette,
        const bbox& box,
        const vox_mat3& rotation,
        const vox_vec3& translation
)
{
	std::shared_ptr<UnvoxMesh> mesh = std::make_shared<UnvoxMesh>();

	// 1) Build raw voxel-space verts & indices
	struct Vertex 
	{
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
	struct VertKey 
	{
		int px_i, py_i, pz_i;
		int nx_i, ny_i, nz_i;
		int u_i, v_i;
		uint32_t colorIndex;
		bool operator==(VertKey const& o) const 
		{
			return px_i == o.px_i && py_i == o.py_i && pz_i == o.pz_i
				&& nx_i == o.nx_i && ny_i == o.ny_i && nz_i == o.nz_i
				&& u_i == o.u_i && v_i == o.v_i
				&& colorIndex == o.colorIndex;
		}
	};

	struct VertKeyHash 
	{
		size_t operator()(VertKey const& k) const noexcept 
		{
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
			if (auto it = vertMap.find(key); it != vertMap.end()) 
			{
				return it->second;
			}
			else 
			{
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
	for (auto& face : faces) 
	{
		// atlas UVs
		float u0 = (face.atlasX + border) * pixelW;
		float v0 = 1.0f - (face.atlasY + border) * pixelH;
		float u1 = (face.atlasX + border + face.w) * pixelW;
		float v1 = 1.0f - (face.atlasY + border + face.h) * pixelH;

		// face normal + 4 corners
		float nx = 0, ny = 0, nz = 0;
		float x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
		switch (face.orientation) 
		{
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
		const vox_vec3 P0{ x0,y0,z0 }, P1{ x1,y1,z1 }, P2{ x2,y2,z2 };
		const vox_vec3 triN = cross(P1 - P0, P2 - P0);
		const bool baseIsCCW = (dot(triN, vox_vec3{ nx,ny,nz }) > 0.0f);
		const bool finalIsCCW = shouldInvert ? !baseIsCCW : baseIsCCW;

		if (finalIsCCW)
		{
			indices.insert(indices.end(), { i0,i1,i2,  i0,i2,i3 });
		}
		else 
		{
			indices.insert(indices.end(), { i0,i2,i1,  i0,i3,i2 });
		}
	}

	// 3) If smooth shading, normalize summed normals
	if (!flatShading) 
	{
		for (auto& v : verts) 
		{
			float L = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
			if (L > 0) { v.nx /= L; v.ny /= L; v.nz /= L; }
		}
	}

	// 4) Upload into aiMesh (unchanged)
	// mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
	// mesh->mNumVertices = (unsigned int)verts.size();
	// mesh->mVertices = new aiVector3D[verts.size()];
	// mesh->mNormals = new aiVector3D[verts.size()];
	// mesh->mTextureCoords[0] = new aiVector3D[verts.size()];
	// mesh->mNumUVComponents[0] = 2;

	mesh->Vertices.resize(verts.size());
	mesh->Normals.resize(verts.size());
	mesh->UVs.resize(verts.size());

	// compute pivot...
	vox_vec3 pivot =
	{
		(box.minX + box.maxX) * 0.5f,
		(box.minY + box.maxY) * 0.5f,
		(box.minZ + box.maxZ) * 0.5f
	};

	for (unsigned int i = 0; i < verts.size(); ++i) 
	{
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
		vox_vec3 pos{ x, z, y };
		vox_vec3 norm{ nx, nz, ny };

		// translation w/ Y⇄Z swap, then un-mirror X
		pos.x += translation.x; pos.y += translation.z; pos.z += translation.y;
		pos.x = -pos.x;  norm.x = -norm.x;

		mesh->Vertices[i] = pos;
		mesh->Normals[i] = norm;
		mesh->UVs[i] = { verts[i].u, verts[i].v };
	}

	mesh->Faces.resize(static_cast<u32>((indices.size() / 3)));

	for (unsigned int f = 0; f < mesh->Faces.size(); ++f) 
	{
		auto& faceOut = mesh->Faces[f];
		
		faceOut.Indices =
		{
			indices[3 * f + 0],
			indices[3 * f + 1],
			indices[3 * f + 2]
		};
	}

	return mesh;
}

}