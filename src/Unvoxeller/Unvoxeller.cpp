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
#include <Unvoxeller/FaceRect.h>
#include <assimp/postprocess.h>
// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <Unvoxeller/VoxParser.h>
#include <Unvoxeller/Log/Log.h>
#include <Unvoxeller/VertexMerger.h>
#include <Unvoxeller/Unvoxeller.h>
//#include <Unvoxeller/ScenePostprocessing.h>
#include <Unvoxeller/MeshBuilder.h>

// Include stb_image_write for saving texture atlas as PNG
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>


#include <Unvoxeller/Mesher/MesherFactory.h>
#include <Unvoxeller/TextureGenerators/TextureGeneratorFactory.h>


// Assume the Unvoxeller namespace and structures from the provided data structure are available:
namespace Unvoxeller
{
	std::unique_ptr<MesherFactory> _mesherFactory = nullptr;
	std::unique_ptr<TextureGeneratorFactory> _textureGeneratorFactory = nullptr;

	Unvoxeller::Unvoxeller()
	{
		_mesherFactory = std::make_unique<MesherFactory>();
		_textureGeneratorFactory = std::make_unique<TextureGeneratorFactory>();
	}

	Unvoxeller::~Unvoxeller()
	{
		
	}

static vox_mat3 decode_rotation(uint8_t r)
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

// Create and save a PNG texture from the atlas data
static bool SaveAtlasImage(const std::string& filename, int width, int height, const std::vector<unsigned char>& rgbaData)
{
	// Use stb_image_write to write PNG
	if (stbi_write_png(filename.c_str(), width, height, 4, rgbaData.data(), width * 4) == 0) {
		return false;
	}
	return true;
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

bbox ComputeMeshBoundingBox(const UnvoxMesh* mesh) 
{
	bbox boundingBox = 
	{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()
	};
	if (!mesh || mesh->Vertices.size() == 0)
		return boundingBox;

	for (unsigned i = 0; i < mesh->Vertices.size(); ++i) 
	{
		const vox_vec3& v = mesh->Vertices[i];
		const vox_vec3& n = mesh->Normals[i];
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



// TODO: start simple, from the begining, the whole code base has a problem of code duplication.
static std::shared_ptr<UnvoxScene> GetModels(const vox_file* voxData, const s32 frameIndex, const std::string& outputPath, const ConvertOptions& options)
{
	struct MeshWrapData
	{
		std::shared_ptr<UnvoxMesh> Mesh;
		std::string imageName;
	};

	std::vector<MeshWrapData> meshes;
	std::vector<std::shared_ptr<UnvoxNode>> shapeNodes = {};

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

			faces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[modelId], voxData->sizes[modelId], modelId);

			// --- Below

			
				mergedFaces.insert(mergedFaces.end(), faces.begin(), faces.end());
			}

			if(options.Texturing.GenerateTextures)
			{
				textureData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(mergedFaces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);
			}
			else
			{
				textureData = nullptr;
			}

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
				faces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[modelId], voxData->sizes[modelId], modelId);

				if(options.Texturing.GenerateTextures)
				{
					textureData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(faces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);
				}
				else
				{
					textureData = nullptr;
				}
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
			auto mesh = MeshBuilder::BuildMeshFromFaces(
				faces,
				textureData->Width, textureData->Height,
				options.Meshing.FlatShading,
				pallete,
				box,          // pivot centering
				wxf.rot,     // MagicaVoxel 3×3 rotation
				wxf.trans    // MagicaVoxel translation
			);

			// 3) Recenter every vertex so that the mesh’s center is at the origin


			// Assign material index...

			if (options.Meshing.GenerateMaterials)
			{
				mesh->MaterialIndex = options.Meshing.MaterialPerMesh && !options.ExportMeshesSeparatelly ? materialIndex++ : 0;
			}
			else
			{
				mesh->MaterialIndex = 0;
			}
			// Record mesh index and create node with identity transform:
			unsigned int meshIndex = static_cast<unsigned int>(meshes.size());

			auto node = std::make_shared<UnvoxNode>();
			node->Name = name;
			
			if (options.ExportMeshesSeparatelly)
			{
				node->MeshesIndexes.push_back(0);
			}
			else
			{
				node->MeshesIndexes.push_back(meshIndex);
			}

			// box = TransformAABB(box, wxf.rot, wxf.trans);
			//  float cxx = box.minX + (box.maxX - box.minX) * options.Pivot.x;
			//  float cyy = box.minY + (box.maxY - box.minY) * options.Pivot.y;
			//  float czz = box.minZ + (box.maxZ - box.minZ) * options.Pivot.z;

			auto bbbox = ComputeMeshBoundingBox(mesh.get());
			float cxx = bbbox.minX + (bbbox.maxX - bbbox.minX) * currentPivot.x;
			float cyy = bbbox.minY + (bbbox.maxY - bbbox.minY) * currentPivot.y;
			float czz = bbbox.minZ + (bbbox.maxZ - bbbox.minZ) * currentPivot.z;

			vox_vec3 cent(cxx, cyy, czz);

			if (options.Meshing.MeshesToWorldCenter)
			{
				for (unsigned int i = 0; i < mesh->Vertices.size(); ++i)
				{
					mesh->Vertices[i] -= cent;
				}
			}
			else
			{
				for (unsigned int i = 0; i < mesh->Vertices.size(); ++i)
				{
					mesh->Vertices[i] -= cent;
				}

				// aiMatrix4x4 C;
				// aiMatrix4x4::Translation(cent, C);

				node->Transform = vox_vec4(cent.x, cent.y,cent.z, 1.0f) * node->Transform;
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


	// if (options.ExportMeshesSeparatelly)
	// {
	// 	for (size_t i = 0; i < meshes.size(); i++)
	// 	{
	// 		auto sceneSplit = std::make_shared<UnvoxScene>();

	// 		// — create & initialize the root node —
	// 		sceneSplit->RootNode = std::make_shared<UnvoxNode>();
	// 		sceneSplit->RootNode->Name = "RootNode";
	// 		sceneSplit->RootNode->Transform = {};   // identity
	// 		// ** zero out the root node’s mesh list **
	// 		sceneSplit->RootNode->MeshesIndexes = {};

	// 		// now attach exactly one child node:
	// 		auto child = std::make_shared<UnvoxNode>();
	// 		child->MeshesIndexes = { 0 };
			
	// 		sceneSplit->RootNode->Children.push_back(child); // Overwrite to set the index to 0

	// 		// — populate the scene’s mesh and material arrays —
	// 		sceneSplit->Meshes = { meshes[i].Mesh };

	// 		if (options.Meshing.GenerateMaterials)
	// 		{
	// 			aiString texPath(meshes[i].imageName);
	// 			aiMaterial* mat = new aiMaterial();
	// 			mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

	// 			// Since the meshes will be exported separatelly, always create a material per mesh
	// 			//sceneSplit->mNumMaterials = 1;
	// 			//sceneSplit->mMaterials = new aiMaterial * [1] { mat };
	// 		}
	// 		else
	// 		{
	// 			//sceneSplit->mNumMaterials = 0;
	// 			//sceneSplit->mMaterials = nullptr;
	// 		}
	// 		scenes.push_back(sceneSplit);
	// 	}
	// }
	// else

	std::shared_ptr<UnvoxScene> scene{};

	{
		scene = std::make_shared<UnvoxScene>();
		scene->RootNode = std::make_shared<UnvoxNode>();
		scene->Meshes.resize(meshes.size());

		// Attach all shape nodes to root
		scene->RootNode->Children.resize(shapeNodes.size());

		// TODO: set one material per mesh, or share a material? for shared materials, the texture individial export option should be turned off, since the material needs the whole atlas.

		if (options.Meshing.GenerateMaterials)
		{
			if (options.Meshing.MaterialPerMesh)
			{
				scene->Materials.resize(meshes.size());
			}
			else
			{
				LOG_ERROR("IMPLEMENT shared materials");
				throw;
				//scene->mNumMaterials = 1;
				//scene->mMaterials = new aiMaterial * [1];

			}
		}
		else
		{
			scene->Materials = {};
		}

		for (size_t i = 0; i < meshes.size(); ++i)
		{
			scene->Meshes[i] = meshes[i].Mesh;

			s32 matIndex = meshes[i].Mesh->MaterialIndex;

			if (options.Meshing.GenerateMaterials)
			{
				aiString texPath(meshes[i].imageName);
				//aiMaterial* mat = new aiMaterial();
				//mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

				auto mat = std::make_shared<UnvoxMaterial>();
				mat->Name =  meshes[i].imageName;
				mat->TextureIndex =  i;

				scene->Materials[matIndex] =  mat;
			}
		}

		// Set all the nodes to the root.
		for (size_t i = 0; i < scene->RootNode->Children.size(); ++i)
		{
			scene->RootNode->Children[i] = shapeNodes[i];
		}
	}

	//for (auto sceneitem : scenes)
	{
		// Adds default material, some formats require at least one.
		// if (sceneitem->Materials.size() == 0)
		// {
		// 	sceneitem->mNumMaterials = 1;
		// 	sceneitem->mMaterials = new aiMaterial * [1] {new aiMaterial};
		// }

		if (options.Meshing.RemoveTJunctions)
		{
			// for (unsigned int i = 0; i < sceneitem->mNumMeshes; ++i)
			// {
			// 	CleanUpMesh(sceneitem->mMeshes[i]);
			// }

			LOG_INFO("Done cleaning up meshes count: {0}", sceneitem->Meshes.size());
		}
	}

	return scene;
}



const std::vector<std::shared_ptr<UnvoxScene>> Run(const vox_file* voxData, const std::string& outputPath, const ConvertOptions& options)
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
		std::vector<std::shared_ptr<UnvoxScene>> scenesOut{};

		for (s32 fi = 0; fi < frameCount; ++fi)
		{
			// Prepare a new minimal scene for this frame
			LOG_INFO("Frame processing: {0}", fi);
			auto scene = GetModels(voxData, fi, outputPath, options);

			// Export this scene
			std::string frameOut = outputPath;
			// Insert frame number before extension
			if (dot != std::string::npos)
			{
				frameOut = outputPath.substr(0, outputPath.find_last_of('.')) + "_frame" + std::to_string(j) + outputPath.substr(outputPath.find_last_of('.'));
			}
			else
			{
				frameOut = outputPath + "_frame" + std::to_string(fi);
			}

			scene->Name = frameOut;

			scenesOut.push_back(scene);
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
		
		if (!options.Texturing.SeparateTexturesPerMesh)
		{
			// If one atlas for all, gather all faces first
			std::vector<FaceRect> allFaces;
			std::unordered_set<uint8_t> usedColors;
			for (size_t i = 0; i < meshCount; ++i) 
			{
				size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1); // This is bad

				std::vector<FaceRect> faces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);
				// Tag faces with an offset or id if needed (not needed for atlas, we just combine)
				allFaces.insert(allFaces.end(), faces.begin(), faces.end());
			}

			const auto texData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(allFaces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);


			// Save atlas image
			std::string baseName = outputPath;
			if (dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
			std::string atlasName = baseName + "_atlas.png";
			SaveAtlasImage(atlasName, texData->Width, texData->Height, texData->Buffer);

			LOG_INFO("Atlas saved");

			// Assign this texture to the single material
			aiString texPath(atlasName);
			scene->mMaterials[0]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
			// Now need to build each mesh's geometry from the portion of faces belonging to that mesh
			// We should separate faces by frame segment: because allFaces is combined list.
			// We can reconstruct segmentation because we processed each frame sequentially and appended.
			// So we can do another loop now generating geometry for each frame from allFaces:

			size_t faceOffset = 0;
			for (size_t i = 0; i < meshCount; ++i) 
			{
				size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1);

				// Remesh the frame to get number of faces:            
				std::vector<FaceRect> frameFaces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);

				// Now copy that many faces from allFaces (they should correspond in order to this frame).
				std::vector<FaceRect> facesForMesh;
				facesForMesh.insert(facesForMesh.end(), allFaces.begin() + faceOffset, allFaces.begin() + faceOffset + frameFaces.size());
				faceOffset += frameFaces.size();
				
				// Create mesh
				//aiMesh* mesh = new aiMesh();

				//scene->mMeshes[i] = mesh;

				auto& sz = voxData->sizes[modelIndex];
				auto& mdl = voxData->voxModels[modelIndex];
				auto  box = mdl.boundingBox;

				auto mesh = MeshBuilder::BuildMeshFromFaces(facesForMesh, texData->Width, texData->Height, options.Meshing.FlatShading, voxData->palette, box);
				LOG_INFO("Build meshes from faces, mesh: {0}", i);

				// TODO: position origin issue, take into account the position of the objects, this should be used, reynardo
				//voxData->transforms.at(0).frameAttrib[0].translation;
				//--voxData->transforms.at(0).frameAttrib[0].rotation;
				//-----


				mesh->MaterialIndex = options.Texturing.SeparateTexturesPerMesh ? (int)i : 0;
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
			// TODO:

			// separateTexturesPerMesh case:
			// for (size_t i = 0; i < meshCount; ++i)
			// {
			// 	size_t modelIndex = (i < voxData->voxModels.size() ? i : voxData->voxModels.size() - 1);

			// 	std::vector<FaceRect> faces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[modelIndex], voxData->sizes[modelIndex], modelIndex);

			// 	const auto texData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(faces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);
				
			// 	std::string baseName = outputPath;
			// 	if (dot != std::string::npos) baseName = outputPath.substr(0, outputPath.find_last_of('.'));
			// 	std::string imageName = baseName + "_mesh" + std::to_string(i) + ".png";
			// 	SaveAtlasImage(imageName,texData->Width, texData->Height, texData->Buffer);

			// 	aiString texPath(imageName);
			// 	scene->mMaterials[i]->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
			// 	// Create mesh geometry
			// 	aiMesh* mesh = new aiMesh();
			// 	scene->Meshes[i] = mesh;

			// 	auto& sz = voxData->sizes[modelIndex];
			// 	auto& mdl = voxData->voxModels[modelIndex];
			// 	auto  box = mdl.boundingBox;

			// 	auto mesh = MeshBuilder::BuildMeshFromFaces(faces, texData->Width, texData->Height, options.Meshing.FlatShading, voxData->palette, box);

			// 	mesh->mMaterialIndex = (int)i;
			// 	// Node
			// 	aiNode* node = new aiNode();
			// 	node->mName = aiString("Mesh" + std::to_string(i));
			// 	node->mNumMeshes = 1;
			// 	node->mMeshes = new unsigned int[1];
			// 	node->mMeshes[0] = i;
			// 	aiMatrix4x4 rot;
			// 	aiMatrix4x4::RotationX(-static_cast<float>(AI_MATH_PI / 2.0f), rot);
			// 	node->mTransformation = rot;

			// 	scene->mRootNode->mChildren[i] = node;
			// }
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
	if (!options.ExportFramesSeparatelly) 
	{
		// Clean materials (if unique)
		std::unordered_set<aiMaterial*> uniqueMats;

		for (unsigned int i = 0; i < scene->mNumMaterials; ++i) 
		{
			uniqueMats.insert(scene->mMaterials[i]);
		}

		for (aiMaterial* mat : uniqueMats) 
		{
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

	void VoxellerInit()
    {
        VoxellerApp::init();
    }
    
    ExportResults Unvoxeller::ExportVoxToModel(const std::string& inVoxPath, const std::string& outExportPath, const ExportOptions& options)
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


    ExportResults Unvoxeller::ExportVoxToModel(const char* buffer, int size, const ExportOptions& options)
    {
        LOG_ERROR("Not implemented");
        throw;

        return {};
    }

    MemData Unvoxeller::VoxToMem(const std::string& inVoxPath, const ConvertOptions& options)
    {
        LOG_ERROR("Not implemented");
        throw;

        return {};
    }

    MemData Unvoxeller::VoxToMem(const char* buffer, int size, const ConvertOptions& options)
    {
        LOG_ERROR("Not implemented");
        throw;
        return {};
    }

    void Unvoxeller::ExportVoxToModelAsync(const char* buffer, int size, const ExportOptions& options, std::function<void(ExportResults)> callback)
    {
        LOG_ERROR("Not implemented");
        throw;
    }

    void Unvoxeller::GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(MemData)> callback)
    {
        LOG_ERROR("Not implemented");
        throw;
    }

}