#include "AssimpSceneWritter.h"
#include <assimp/postprocess.h>
// Include Assimp headers for creating and exporting 3D assets
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/material.h>
#include <Unvoxeller/Log/Log.h>

namespace Unvoxeller
{
    static std::unique_ptr<aiScene> GetAssimpScene(const ConvertOptions& options, const UnvoxScene* unvoxScene)
    {
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

        return nullptr;
    }
    
    bool AssimpSceneWritter::ExportScenes(const ExportOptions& options, const std::vector<std::shared_ptr<UnvoxScene>> scenes)
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

	size_t dot = options.OutputPath.find_last_of('.');

	for (size_t i = 0; i < scenes.size(); i++)
	{
		auto scene = scenes[i];

		//OptimizeAssimpScene(scene);

		if (options.Converting.Meshing.WeldVertices)
		{
			preprocess = aiProcess_JoinIdenticalVertices;
			// for (size_t i = 0; i < scene->mNumMeshes; i++)
			// {
			// 	MergeIdenticalVertices(scene->mMeshes[i]);
			// }
		}

		const std::string convertedOutName = options.OutputPath.substr(0, dot) + (scenes.size() > 1 ? "_" + std::to_string(i) : "");
		//--scene->RootNode->Transform = scaleMat * scene->RootNode->Transform;

		aiReturn ret = exporter.Export(GetAssimpScene(options.Converting, scene.get()).get(), formatId.c_str(), convertedOutName + "." + ext, preprocess);

		if (ret != aiReturn_SUCCESS)
		{
			LOG_ERROR("Export failed: {0}", exporter.GetErrorString());
			return false;
		}

	}

	return true;

    }
}