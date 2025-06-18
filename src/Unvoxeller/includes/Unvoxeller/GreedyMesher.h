#pragma once
#include <string>
#include <vector>
#include <functional>
#include <Unvoxeller/api.h>
#include <Unvoxeller/Math/VoxVector.h>

namespace Unvoxeller
{
	enum class ModelFormat
	{
		FBX, OBJ,
	};

	struct UNVOXELLER_API MeshInfo
	{
		s32 Vertices;
		s32 Indices;
		s32 edges;
	};

	enum class ConvertMSG
	{
		FAILED,
		SUCESS,
	};

	enum class TextureFormat
	{
		// Raw texture: 4 channels, 4 bytes per pixel, 8 bits per channel.
		RGBA8,
		// Encode as .png
		PGN,
		// Encode as .jpg
		JPG,
		// Encode as .tga
		TGA,
	};

	enum class TextureType
	{
		// Will generate performant meshes, but texture will be more complex and bigger in size. 
		// Perfect for complex meshes. Bake supported.
		Atlas,

		// Will generate a not so performant meshes, and a no so performant texture, but final mesh fidelity will be higher. 
		// Great for non interactive media (videos/images). Bake supported.
		AtlasHighFidelity,

		// Will generate meshes with more complex topology but simpler and smaller texture made of individual colors per pixel. 
		// Perfect for simple meshes. Baking is not possible/harder to do.
		Palette
	};

	struct UNVOXELLER_API PaletteTextureConfig
	{
		bool UseColumns = false;

		// How many pixels can be fit in the same Row/Column before increasing the texture dimension.
		s32 Max = 255;
	};

	struct UNVOXELLER_API TextureData
	{
		std::vector<unsigned char> Buffer;
		TextureFormat Format;
		f32 Width;
		f32 Height;
	};

	struct UNVOXELLER_API ConvertResult
	{
		ConvertMSG Msg;
		std::vector<MeshInfo> meshes;

		std::vector<TextureData> Textures;
	};

	struct UNVOXELLER_API ExportResults
	{
		ConvertResult Convert = {};
		std::string OutPath = "";
	};

	struct UNVOXELLER_API ConvertOptions
	{
		// Remove T-Juntions
		bool RemoveTJunctions = false;

		// Make Textures to always be power of two
		bool TexturesPOT = false;

		// Should every mesh have a separated texture?
		bool SeparateTexturesPerMesh = false;

		// Vertices will be shared across faces.
		bool WeldVertices = false;

		// Prevent smoothing the normals.
		bool FlatShading = true;

		bool ExportFramesSeparatelly = true; // Move this to export options

		// Export meshes in different files.
		bool ExportMeshesSeparatelly = false; // Move this to export options

		bool MaterialPerMesh = true;

		// Overwrite meshes positions, this will put all the meshes in the center of the world (0, 0, 0)
		// If a file has multiples meshes, then all their position will be (0, 0, 0)
		bool MeshesToWorldCenter = false;

		// Generate All Materials, otherwise a default (empty) one will be used for all meshes.
		bool GenerateMaterials = true;

		// 
		bool GenerateTextures = true;

		// Parts of meshes that are not visible will be removed.
		bool RemoveOccludedFaces = false;

		// All meshes will be merged, this will overwrite texture export options, since an atlas will be exported. 
		// Tip: use it alongside "RemoveOcludedFaces" for cleaner and performat results.
		bool MergeMeshes = false;

		// If true:
		// -When exporting separated textures per mesh, materials will reuse the textures files (Ex: two materials could point to the same texture)
		// -When using texture atlas, faces will reuse the same uv locations.
		// Note: Choose this option if No baking will be done on the final meshes. 
		bool OptimizeTextures = false;

		TextureType TextureType = TextureType::Atlas;

		// Scale, Ex, if 1.0, every single voxel will take up 1 unit.
		vox_vec3 Scale = { 1.0f, 1.0f, 1.0f };

		vox_vec3 Pivot{ 0.5f, 0.5f, 0.5f };
	};

	struct UNVOXELLER_API ExportOptions
	{
		ConvertOptions ConvertOptions;
		ModelFormat OutputFormat = ModelFormat::FBX;
	};

	struct UNVOXELLER_API MemData
	{
		ConvertResult Convert;
	};

	class UNVOXELLER_API GreedyMesher
	{
	public:
		static ExportResults ExportVoxToModel(const std::string& inVoxPath, const std::string& outExportPath, const ExportOptions& options);
		static ExportResults ExportVoxToModel(const char* buffer, int size, const ExportOptions& options);

		static MemData VoxToMem(const std::string& inVoxPath, const ConvertOptions& options);
		static MemData VoxToMem(const char* buffer, int size, const ConvertOptions& options);


		static void ExportVoxToModelAsync(const char* buffer, int size, const ExportOptions& options, std::function<void(ExportResults)> callback);
		static void GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(MemData)> callback);

	private:
	};
}
