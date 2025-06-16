#include <Voxeller/VertexMerger.h>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <cmath>
#include <assimp/scene.h>      // aiMesh, aiVector3D, aiFace

// A simple hashable key for pos+norm+uv
struct VertKey {
	aiVector3D p, n, uv;
	bool operator==(VertKey const& o) const {
		return p == o.p && n == o.n && uv == o.uv;
	}
};

// Hash function for our key
struct VertKeyHash {
	size_t operator()(VertKey const& k) const {
		auto h1 = std::hash<float>()(k.p.x) ^ (std::hash<float>()(k.p.y) << 1) ^ (std::hash<float>()(k.p.z) << 2);
		auto h2 = std::hash<float>()(k.n.x) ^ (std::hash<float>()(k.n.y) << 1) ^ (std::hash<float>()(k.n.z) << 2);
		auto h3 = std::hash<float>()(k.uv.x) ^ (std::hash<float>()(k.uv.y) << 1);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

void MergeIdenticalVertices(aiMesh* mesh) {
	using namespace std;
	unordered_map<VertKey, unsigned int, VertKeyHash> mapVertex;
	vector<aiVector3D> newPos, newNorm, newUV;
	vector<unsigned int> remap(mesh->mNumVertices);

	bool hasNorm = mesh->HasNormals();
	bool hasUV = mesh->HasTextureCoords(0);

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		VertKey key;
		key.p = mesh->mVertices[i];
		key.n = hasNorm ? mesh->mNormals[i] : aiVector3D{};
		key.uv = hasUV ? mesh->mTextureCoords[0][i] : aiVector3D{};
		auto it = mapVertex.find(key);
		if (it == mapVertex.end()) {
			unsigned int n = (unsigned int)newPos.size();
			mapVertex[key] = n;
			newPos.push_back(key.p);
			if (hasNorm) newNorm.push_back(key.n);
			if (hasUV)   newUV.push_back(key.uv);
			remap[i] = n;
		}
		else {
			remap[i] = it->second;
		}
	}

	unsigned int uniqueCount = (unsigned int)newPos.size();

	// Reallocate arrays
	delete[] mesh->mVertices;
	mesh->mVertices = new aiVector3D[uniqueCount];
	mesh->mNumVertices = uniqueCount;

	if (hasNorm) {
		delete[] mesh->mNormals;
		mesh->mNormals = new aiVector3D[uniqueCount];
	}
	if (hasUV) {
		delete[] mesh->mTextureCoords[0];
		mesh->mTextureCoords[0] = new aiVector3D[uniqueCount];
		mesh->mNumUVComponents[0] = 2;
	}

	// Copy new vertex data
	for (unsigned int i = 0; i < uniqueCount; ++i) {
		mesh->mVertices[i] = newPos[i];
		if (hasNorm) mesh->mNormals[i] = newNorm[i];
		if (hasUV)   mesh->mTextureCoords[0][i] = newUV[i];
	}

	// Fix all face indices
	for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
		aiFace& face = mesh->mFaces[f];
		for (unsigned j = 0; j < face.mNumIndices; ++j) {
			face.mIndices[j] = remap[face.mIndices[j]];
		}
	}
}
