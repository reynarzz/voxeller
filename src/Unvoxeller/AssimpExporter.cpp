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
	static std::vector<aiScene*> GetAssimpScene(const std::string& name, const ConvertOptions& options, const std::vector<std::shared_ptr<UnvoxScene>>& unvoxScenes)
	{
		aiMatrix4x4 scaleMat;
		aiMatrix4x4::Scaling(aiVector3D(options.Scale.x, options.Scale.y, options.Scale.z), scaleMat);

		std::vector<aiScene*> scenesOut(unvoxScenes.size());

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

		for (size_t i = 0; i < unvoxScenes.size(); i++)
		{
			/* code */
			const auto& scene = unvoxScenes[i];

		if (!options.ExportMeshesSeparatelly)
		{
			auto sceneOut = new aiScene();

			// � create & initialize the root node �
			sceneOut->mRootNode = new aiNode();
			sceneOut->mRootNode->mName = "RootNode";
			sceneOut->mRootNode->mTransformation = {};   // identity
			// ** zero out the root node�s mesh list **
			sceneOut->mRootNode->mNumMeshes = 0;
			sceneOut->mNumMeshes = scene->Meshes.size();
			sceneOut->mMeshes = new aiMesh*[scene->Meshes.size()];
			
			for (size_t i = 0; i < scene->Meshes.size(); i++)
			{
				auto child = new aiNode();
				child->mNumMeshes = 1;
				sceneOut->mRootNode->addChildren(1, &child); // Overwrite to set the index to 0
				child->mMeshes = new u32[1] { static_cast<u32>(i) };

				const auto mesh = scene->Meshes[i];

				aiMesh* meshOut = new aiMesh();
				
				meshOut->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
				meshOut->mNumVertices = (unsigned int)mesh->Vertices.size();
				meshOut->mNumFaces = mesh->Faces.size();
				meshOut->mFaces = new aiFace[mesh->Faces.size()];
				meshOut->mVertices = new aiVector3D[mesh->Vertices.size()];
				meshOut->mNormals = new aiVector3D[mesh->Normals.size()];
				meshOut->mTextureCoords[0] = new aiVector3D[mesh->UVs.size()];
				meshOut->mNumUVComponents[0] = 2;
				
				sceneOut->mMeshes[i] = meshOut;

				for (size_t i = 0; i < mesh->Vertices.size(); i++)
				{
					const auto& vert = mesh->Vertices[i];
					meshOut->mVertices[i] = { vert.x, vert.y, vert.z };
				}
				
				for (size_t i = 0; i < mesh->Normals.size(); i++)
				{
					const auto& norm = mesh->Normals[i];
					meshOut->mNormals[i] = { norm.x, norm.y, norm.z };
				}

				for (size_t i = 0; i < mesh->Faces.size(); i++)
				{
					const auto& face = mesh->Faces[i];
					aiFace oFace;
					oFace.mNumIndices = static_cast<u32>(face.Indices.size());
					oFace.mIndices = new u32[face.Indices.size()];
					std::copy(face.Indices.begin(), face.Indices.end(), oFace.mIndices);
					meshOut->mFaces[i] = oFace;
				}

				for (size_t i = 0; i < mesh->UVs.size(); i++)
				{
					const auto& uv = mesh->UVs[i];
					meshOut->mTextureCoords[0][i] = { uv.x, uv.y, 0 };
				}

				if (options.Meshing.GenerateMaterials)
				{
					const aiString texPath("Output.png");//(scene->Textures[scene->Materials[mesh->MaterialIndex]->TextureIndex]->Name.data());
					
					aiMaterial* mat = new aiMaterial();
					mat->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

					// Since the meshes will be exported separatelly, always create a material per mesh
					sceneOut->mNumMaterials = 1;
					sceneOut->mMaterials = new aiMaterial * [1] { mat };
				}
				else
				{
					sceneOut->mNumMaterials = 0;
					sceneOut->mMaterials = nullptr;
				}
			}

			scenesOut[i] = sceneOut;
		}
		}

		return scenesOut;
	}

	bool AssimpSceneWritter::Export(const ExportOptions& options, const ConvertOptions& cOptions, const std::vector<std::shared_ptr<UnvoxScene>>& scenes)
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
			return false;
		}

		Assimp::Exporter exporter;
		const aiExportFormatDesc* selectedFormat = nullptr;
		for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i) 
		{
			const aiExportFormatDesc* fmt = exporter.GetExportFormatDescription(i);

			LOG_INFO("- {0}", fmt->fileExtension);

			if (fmt && fmt->fileExtension == ext) 
			{
				selectedFormat = fmt;
				break;
			}
		}
		
		if (!selectedFormat)
		{
			LOG_ERROR("Unsupported export format: {0}", ext);
			return false;
		}

		u32 preprocess = 0;

		const auto assimpScenes = GetAssimpScene(options.OutputName, cOptions, scenes);

		for (size_t i = 0; i < assimpScenes.size(); i++)
		{
			const std::string convertedOutName = options.OutputName + (assimpScenes.size() > 1 ? "_" + std::to_string(i) : "");

			const std::string outPath = options.OutputDir + "/" + convertedOutName + "." + ext;
			LOG_INFO("Export begin: '{0}'", convertedOutName);
			//--scene->RootNode->Transform = scaleMat * scene->RootNode->Transform;

			aiReturn ret = exporter.Export(assimpScenes[i], selectedFormat->id, outPath, preprocess);

			delete assimpScenes[i];
			
			if (ret != aiReturn_SUCCESS)
			{
				LOG_ERROR("Export failed: {0}", exporter.GetErrorString());
				return false;
			}
			else
			{
				LOG_INFO("Export '{0}' success", convertedOutName);
			}
		}

		return true;

	}
}