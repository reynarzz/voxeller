#include <Unvoxeller/AssimpSceneWritter.h>
#include <assimp/postprocess.h>
// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <Unvoxeller/Log/Log.h>

namespace Unvoxeller
{
	static std::vector<std::unique_ptr<aiScene>> GetAssimpScene(const ConvertOptions& options, const std::vector<std::shared_ptr<UnvoxScene>>& unvoxScenes)
	{
		aiMatrix4x4 scaleMat;
		aiMatrix4x4::Scaling(aiVector3D(options.Scale.x, options.Scale.y, options.Scale.z), scaleMat);

		if (options.Texturing.SeparateTexturesPerMesh)
		{
			// each mesh gets its own material
			// for (size_t i = 0; i < meshCount; ++i) 
			// {
			// 	scene->Materials[i] = new aiMaterial();
			// }
		}
		else
		{
			// one material for all meshes
			// scene->mMaterials[0] = new aiMaterial();
			// for (size_t i = 1; i < meshCount; ++i) 
			// {
			// 	scene->mMaterials[i] = scene->mMaterials[0];
			// }
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

		return {};
	}

	bool AssimpSceneWritter::Export(const ExportOptions& options, const std::vector<std::shared_ptr<UnvoxScene>>& scenes)
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

		const u32 dot = static_cast<u32>(options.OutputPath.find_last_of('.'));

		const auto assimpScenes = GetAssimpScene(options.Converting, scenes);

		for (size_t i = 0; i < assimpScenes.size(); i++)
		{
			const std::string convertedOutName = options.OutputPath.substr(0, dot) + (assimpScenes.size() > 1 ? "_" + std::to_string(i) : "");

			//--scene->RootNode->Transform = scaleMat * scene->RootNode->Transform;

			aiReturn ret = exporter.Export(assimpScenes[i].get(), formatId.c_str(), convertedOutName + "." + ext, preprocess);

			if (ret != aiReturn_SUCCESS)
			{
				LOG_ERROR("Export failed: {0}", exporter.GetErrorString());
				return false;
			}

		}

		return true;

	}
}