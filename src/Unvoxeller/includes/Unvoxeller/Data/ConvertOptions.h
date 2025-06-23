#pragma once

#include <Unvoxeller/api.h>
#include <Unvoxeller/Types.h>
#include <vector>
#include <glm/vec3.hpp>

namespace Unvoxeller 
{
	
	enum class ModelFormat
	{
		FBX, OBJ,
	};

	enum class TextureType
	{
		// Will generate performant meshes, but texture will be more complex and bigger in size. 
		// Perfect for complex meshes. Bake supported.
		Atlas,

		//// Will generate a not so performant meshes, and a no so performant texture, but final mesh fidelity will be higher. 
		//// Great for non interactive media (videos/images). Bake supported.
		//AtlasHighFidelity,

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

	
	struct UNVOXELLER_API TexturingOptions
	{
		bool GenerateTextures = true;

		TextureType TextureType = TextureType::Atlas;

		// Make Textures to always be power of two
		bool TexturesPOT = false;

		// Should every mesh have a separated texture?
		bool SeparateTexturesPerMesh = false;

		// Export smaller textures by modifying UV mapping so islands can reuse spaces of same colors. 
		// NOTE: Baking with this uv mapping, could produce issues since uv islands are "merged" by color.
		bool ReuseColors = false;

		bool ReuseTextures = false;
		// If true:
		// -When exporting separated textures per mesh, materials will reuse the textures files (Ex: two materials could point to the same texture)
		// -When using texture atlas, faces will reuse the same uv locations.
		// Note: Choose this option if No baking will be done on the final meshes. 
		bool OptimizeTextures = false;

	};

	enum VisibleSides
	{
		FRONT   = 1 << 0,
		BACK    = 1 << 1,
		TOP     = 1 << 2,
		BOTTOM  = 1 << 3,
		RIGHT   = 1 << 4,
		LEFT    = 1 << 5,
		ALL     = FRONT | BACK | TOP | BOTTOM | RIGHT | LEFT
	};

	struct UNVOXELLER_API MeshingOptions
	{
		// Remove T-Juntions
		bool RemoveTJunctions = false;

		// Vertices will be shared across faces.
		bool WeldVertices = false;

		// Prevent smoothing the normals.
		bool FlatShading = true;

		bool MaterialPerMesh = true;

		// Generate All Materials, otherwise a default (empty) one will be used for all meshes.
		bool GenerateMaterials = true;

		// Overwrite meshes positions, this will put all the meshes in the center of the world (0, 0, 0)
		// If a file has multiples meshes, then all their position will be (0, 0, 0)
		bool MeshesToWorldCenter = false;

		// Parts of meshes that are not visible will be removed.
		bool RemoveOccludedFaces = false;

		// All meshes will be merged, this will overwrite texture export options, since an atlas will be exported. 
		// Tip: use it alongside "RemoveOcludedFaces" for cleaner and performat results.
		bool MergeMeshes = false;

		VisibleSides Sides;
	};


	struct UNVOXELLER_API ConvertOptions
	{
		ConvertOptions() : Pivots({})
		{
			Meshing.FlatShading = true;
			Meshing.Sides = VisibleSides::ALL;
		}

		bool ExportFramesSeparatelly = true; // Move this to export options

		// Export meshes in different files.
		bool ExportMeshesSeparatelly = false; // Move this to export options

		MeshingOptions Meshing{};

		// Texturing options
		TexturingOptions Texturing{};

		// To create n lods set factors (0.0 - 1.0), 1.0 = no change, Factor[0] = LOD1, Factor[1] = LOD2
		std::array<f32, 10> Lods = { 0,0,0,0,0,0,0,0,0,0 };

		// Scale, Ex, if 1.0, every single voxel will take up 1 unit.
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		// Set pivots for every mesh indexwise, if only one is added, it will be shared across meshes.
		std::vector<glm::vec3> Pivots = {};
	};
}