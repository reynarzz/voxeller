#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <cassert>
#include <Unvoxeller/Unvoxeller.h>
#include <Unvoxeller/FaceRect.h>
#include <Unvoxeller/VoxParser.h>
#include <Unvoxeller/Log/Log.h>
#include <Unvoxeller/VertexMerger.h>
#include <Unvoxeller/ScenePostprocessing.h>
#include <Unvoxeller/AssimpSceneWritter.h>
#include <Unvoxeller/Mesher/MesherFactory.h>
#include <Unvoxeller/TextureGenerators/TextureGeneratorFactory.h>
#include <Unvoxeller/MeshBuilder.h>
#include <stb/stb_image_write.h>

// Assume the Unvoxeller namespace and structures from the provided data structure are available:
namespace Unvoxeller
{
	std::unique_ptr<MesherFactory> _mesherFactory = nullptr;
	std::unique_ptr<TextureGeneratorFactory> _textureGeneratorFactory = nullptr;
	std::unique_ptr<AssimpSceneWritter> _assimpWriter = nullptr;


	Unvoxeller::Unvoxeller()
	{
		_mesherFactory = std::make_unique<MesherFactory>();
		_textureGeneratorFactory = std::make_unique<TextureGeneratorFactory>();
		_assimpWriter = std::make_unique<AssimpSceneWritter>();
	}

	Unvoxeller::~Unvoxeller()
	{
	}

	inline vox_vec3 Rotate3x3(const vox_mat3& m, const vox_vec3& v)
{
    // Use the same convention everywhere (this matches your earlier Rotate helper):
    return {
        m.m00 * v.x + m.m10 * v.y + m.m20 * v.z,
        m.m01 * v.x + m.m11 * v.y + m.m21 * v.z,
        m.m02 * v.x + m.m12 * v.y + m.m22 * v.z
    };
}

// Accumulates world transform for a shape node, correcting Magica's center-vs-plane offset
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

    // 2) Walk up to the root; push parent->child order onto a stack
    std::stack<vox_transform> xfStack;

    while (trnID != -1) {
        auto it = voxData.transforms.find(trnID);
        if (it == voxData.transforms.end()) break;
        const vox_nTRN& trn = it->second;

        // clamp frameIndex
        int fidx = (frameIndex < trn.framesCount ? frameIndex : 0);
        const vox_frame_attrib& attr = trn.frameAttrib[fidx];

        // --- Half-voxel correction (once per node, in Magica space) ---
        // Magica's transforms operate about voxel centers; our faces lie on planes.
        // Subtract R * (0.5, 0.5, 0.5) from the node translation.
        const vox_vec3 half{ 0.5f, 0.5f, 0.5f };
        const vox_vec3 bias = Rotate3x3(attr.rotation, half);
        vox_vec3 correctedT{
            attr.translation.x - bias.x,
            attr.translation.y - bias.y,
            attr.translation.z - bias.z
        };

        xfStack.push(vox_transform(attr.rotation, correctedT));

        // 3) find the *parent* transform of this nTRN:
        //    a) check explicit "_parent" style attributes
        int parentID = -1;
        for (auto const& key : { "_parent", "_parent_id", "_parentID" }) {
            auto ait = trn.attributes.find(key);
            if (ait != trn.attributes.end()) {
                parentID = std::stoi(ait->second);
                break;
            }
        }

        //    b) if none, see if this nTRN appears as a child of an nGRP, and walk up from that group
        if (parentID < 0) {
            for (auto const& gkv : voxData.groups) {
                for (int cid : gkv.second.childrenIDs) {
                    if (cid == trnID) {
                        parentID = findTransformByChild(gkv.first);
                        break;
                    }
                }
                if (parentID >= 0) break;
            }
        }

        trnID = parentID;
    }

    // 4) Multiply in parent→child order
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



	// TODO: start simple, from the begining, the whole code base has a problem of code duplication.
	static std::shared_ptr<UnvoxScene> GetModels(const vox_file* voxData, const s32 frameIndex, const ConvertOptions& options)
	{
		struct MeshWrapData
		{
			std::shared_ptr<UnvoxMesh> Mesh;
			s32 textureIndex;
		};

		std::vector<MeshWrapData> meshes;
		std::vector<std::shared_ptr<UnvoxNode>> shapeNodes = {};
		std::shared_ptr<UnvoxScene> scene = std::make_shared<UnvoxScene>();

		s32 materialIndex = 0;

		const std::vector<vox_vec3>& pivots = options.Pivots;

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



			std::vector<FaceRect> faces;

			std::vector<FaceRect> mergedFaces = {};

			std::unordered_map<s32, std::vector<FaceRect>> modelsData = {};


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

				if (options.Texturing.GenerateTextures)
				{
					textureData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(mergedFaces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);
					scene->Textures.push_back(textureData);

				}
				else
				{
					textureData = nullptr;
				}

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

					if (options.Texturing.GenerateTextures)
					{
						textureData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(faces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);
					}
					else
					{
						textureData = nullptr;
					}
					// Remove this from here
					// if (options.Texturing.GenerateTextures)
					// {
					// 	imageName = baseName + "_frame" + std::to_string(shapeIndex++) + ".png";
					// 	SaveAtlasImage(imageName, textureData->Width, textureData->Height, textureData->Buffer);
					// }

					scene->Textures.push_back(textureData);

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
					voxData->sizes[modelId],
					wxf.rot,     // MagicaVoxel 3×3 rotation
					wxf.trans    // MagicaVoxel translation,
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

				meshes.push_back({ mesh, shapeIndex });
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
			auto mat = std::make_shared<UnvoxMaterial>();
			mat->Name = "Default Material";
			mat->TextureIndex = 0;
			scene->Materials = { mat };
		}

		for (size_t i = 0; i < meshes.size(); ++i)
		{
			scene->Meshes[i] = meshes[i].Mesh;

			s32 matIndex = meshes[i].Mesh->MaterialIndex;

			if (options.Meshing.GenerateMaterials)
			{
				//aiString texPath(meshes[i].imageName);
				//aiMaterial* mat = new aiMaterial();
				//mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

				auto mat = std::make_shared<UnvoxMaterial>();
				//mat->Name =  meshes[i].textureIndex;
				mat->TextureIndex = std::min(i, scene->Textures.size()-1);

				scene->Materials[matIndex] = mat;
			}
		}

		// Set all the nodes to the root.
		for (size_t i = 0; i < scene->RootNode->Children.size(); ++i)
		{
			scene->RootNode->Children[i] = shapeNodes[i];
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

				//LOG_INFO("Done cleaning up meshes count: {0}", sceneitem->Meshes.size());
			}
		}

		return scene;
	}

	const std::vector<std::shared_ptr<UnvoxScene>> Run(const vox_file* voxData, const ConvertOptions& options)
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

		if (options.ExportFramesSeparatelly && frameCount >= 1 && voxData->shapes.size() > 0)
		{
			std::vector<std::shared_ptr<UnvoxScene>> scenesOut{};

			for (s32 fi = 0; fi < frameCount; ++fi)
			{
				// Prepare a new minimal scene for this frame
				LOG_INFO("Frame processing: {0}", fi);
				auto scene = GetModels(voxData, fi, options);

				scenesOut.push_back(scene);
			}

			return scenesOut;
		}
		else
		{
			size_t meshCount = voxData->voxModels.size();

			auto scene = std::make_shared<UnvoxScene>();
			scene->RootNode = std::make_shared<UnvoxNode>();

			scene->RootNode->Children.resize(meshCount);
			scene->Meshes.resize(meshCount);
			scene->Materials.resize((unsigned int)(options.Texturing.SeparateTexturesPerMesh ? meshCount : 1));

			std::vector<FaceRect> allFaces{};

			if (options.Texturing.SeparateTexturesPerMesh)
			{
				
			}
			else 
			{
				// If one atlas for all, gather all faces first
				for (size_t i = 0; i < meshCount; ++i)
				{
					std::vector<FaceRect> faces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[i], voxData->sizes[i], i);
					// Tag faces with an offset or id if needed (not needed for atlas, we just combine)
					allFaces.insert(allFaces.end(), faces.begin(), faces.end());
				}
			}


			LOG_INFO("TODO: Atlas saved");

			// Assign this texture to the single material
			//aiString texPath(atlasName);
			auto oMat = std::make_shared<UnvoxMaterial>();
			//oMat->Name = atlasName;
			oMat->TextureIndex = 0;
			scene->Materials[0] = oMat;

			size_t faceOffset = 0;
			for (size_t i = 0; i < meshCount; ++i)
			{
				// Remesh the frame to get number of faces:            
				std::vector<FaceRect> frameFaces = _mesherFactory->Get(options.Meshing.MeshType)->CreateFaces(voxData->voxModels[i], voxData->sizes[i], i);

				const auto texData = _textureGeneratorFactory->Get(options.Texturing.TextureType)->GetTexture(frameFaces, voxData->palette, voxData->voxModels, options.Texturing.TexturesPOT);

				auto& sz = voxData->sizes[i];
				auto& mdl = voxData->voxModels[i];
				auto& box = mdl.boundingBox;

				auto mesh = MeshBuilder::BuildMeshFromFaces(frameFaces, texData->Width, texData->Height, options.Meshing.FlatShading, voxData->palette, box, sz);
				mesh->MaterialIndex = options.Texturing.SeparateTexturesPerMesh ? (int)i : 0;

				// Create node for this mesh
				auto node = std::make_shared<UnvoxNode>();
				node->Name = "Frame" + std::to_string(i);
				node->MeshesIndexes = { static_cast<s32>(i) };

				scene->RootNode->Children[i] = node;
				scene->Meshes[i] = mesh;
				scene->Textures.push_back(texData);

				LOG_INFO("Completed mesh: {0}", i);
			}

			return { scene };
		}

		return {};
	}
	
	void VoxellerInit()
	{
		VoxellerApp::init();
	}

	ExportResults Unvoxeller::ExportVoxToModel(const ExportOptions& eOptions, const ConvertOptions& cOptions)
	{
		if(eOptions.InputPath.empty())
		{
			LOG_ERROR("Path is empty");

			return { ConvertMSG::ERROR_EMPTY_PATH };
		}

		std::shared_ptr<vox_file> voxData = VoxParser::read_vox_file(eOptions.InputPath.c_str());
		const auto scenes = Run(voxData.get(), cOptions);

		ExportResults results{};
		std::string imageName = "";

		LOG_INFO("About to save texture: {0}", eOptions.OutputName);

		for (const auto& scene : scenes)
		{
			const bool isMultiTexture = scene->Textures.size() > 1;

			for (size_t i = 0; i < scene->Textures.size(); i++)
			{
				const auto& textureData = scene->Textures[i];
				imageName = eOptions.OutputName + (isMultiTexture ? "_" + std::to_string(i) : "") + ".png";
				SaveAtlasImage(eOptions.OutputDir + "/" + imageName, textureData->Width, textureData->Height, textureData->Buffer);
			}
		}	
		
		LOG_INFO("Textures saved");

		// Export this scene
		// std::string frameOut = outputPath;
		// // Insert frame number before extension
		// if (dot != std::string::npos)
		// {
		// 	frameOut = outputPath.substr(0, outputPath.find_last_of('.')) + "_frame" + std::to_string(fi) + outputPath.substr(outputPath.find_last_of('.'));
		// }
		// else
		// {
		// 	frameOut = outputPath + "_frame" + std::to_string(fi);
		// }

		// scene->Name = frameOut;

		if (scenes.size() > 0)
		{
			LOG_INFO("TJuntctions: {0}", cOptions.Meshing.RemoveTJunctions);

			// TODO: This makes the algorithm freeze when a vox has multiple frames, and is exported as no separated
			if (cOptions.Meshing.RemoveTJunctions)
			{
				for (unsigned int m = 0; m < scenes.size(); ++m)
				{
					for (auto& mesh : scenes[m]->Meshes)
					{
						CleanUpMesh(mesh.get());
					}

					LOG_INFO("Done cleaning up meshes count: {0}", scenes[m]->Meshes.size());
				}
			}

			// TODO: fix assimp exporter
			if(_assimpWriter->Export(eOptions, cOptions, scenes))
			{
				results.Msg = ConvertMSG::SUCESS;
			}

		}
		else
		{
			results.Msg = ConvertMSG::FAILED;
		}

		return results;
	}

	ExportResults Unvoxeller::ExportScene(const ExportOptions& eOptions, const ConvertOptions& cOptions, const std::weak_ptr<UnvoxScene> scene)
	{
		ExportResults results{};

		if (_assimpWriter->Export(eOptions, cOptions, { scene.lock() }))
		{
			results.Msg = ConvertMSG::SUCESS;
		}

		return results;
	}


	ExportResults Unvoxeller::ExportVoxToModel(const char* buffer, int size, const ExportOptions& options)
	{
		LOG_ERROR("Not implemented");
		throw;

		return {};
	}

	ConvertResult Unvoxeller::VoxToMem(const std::string& inVoxPath, const ConvertOptions& options)
	{
		std::shared_ptr<vox_file> voxData = VoxParser::read_vox_file(inVoxPath.c_str());
		const auto scenes = Run(voxData.get(), options);
		ConvertResult result{};
		result.Scenes = scenes;
		result.Msg = ConvertMSG::SUCESS;

		return result;
	}

	ConvertResult Unvoxeller::VoxToMem(const char* buffer, int size, const ConvertOptions& options)
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

	void Unvoxeller::GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(ConvertResult)> callback)
	{
		LOG_ERROR("Not implemented");
		throw;
	}

}