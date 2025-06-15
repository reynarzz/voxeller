#pragma once
#include <string>
#include <voxeller/Types.h>
#include <vector>
#include <functional>
#include <voxeller/api.h>

namespace Voxeller
{

enum class ModelFormat
{
    FBX, OBJ, 
};

struct VOXELLER_API MeshInfo
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

struct VOXELLER_API ConvertResult
{
    ConvertMSG Msg;
    std::vector<MeshInfo> meshes;
};

struct VOXELLER_API ExportResults
{
    ConvertResult Convert = {};
    std::string OutPath = "";
};

struct VOXELLER_API ConvertOptions
{
    // Remove T-Juntions
    bool RemoveTJunctions = false;

    bool ExportTextures = true;

    // Make Textures to always be power of two
    bool TexturesPOT = false;

    // Should every mesh have a separated texture?
    bool SeparateTexturesPerMesh = false;

    // Vertices will be shared across faces.
    bool WeldVertices = false;

    // Prevent smoothing the normals.
    bool FlatShading = true;

    bool ExportFramesSeparatelly = true;

    bool ExportMeshesSeparatelly = false;

    bool PreserveMeshesOrigins = true;

    bool MaterialPerMesh = true;

    // Export All Materials, otherwise a default (empty) one will be used for all meshes.
    bool ExportMaterials = true;

    // Parts of meshes that are not visible will be removed.
    bool RemoveOcludedFaces = false;

    // All meshes will be merged, this will overwrite texture export options, since an atlas will be exported. 
    // Tip: use it alongside "RemoveOcludedFaces" for cleaner and performat results.
    bool MergeMeshes = false;

    // Scale, Ex, if 1.0, every single voxel will take up 1 unit.
    f32 Scale = 1.0f;
};

struct VOXELLER_API ExportOptions
{
    ConvertOptions ConvertOptions;
    ModelFormat OutputFormat = ModelFormat::FBX;
};

struct VOXELLER_API MeshingResults
{
    ConvertResult Convert;
};

class VOXELLER_API GreedyMesher
{
public:
    static ExportResults ExportVoxToModel(const std::string& inVoxPath, const std::string& outExportPath, const ExportOptions& options);
    static ExportResults ExportVoxToModel(const char* buffer, int size, const ExportOptions& options);

    static MeshingResults GetModelFromVOXMesh(const std::string& inVoxPath, const ConvertOptions& options);
    static MeshingResults GetModelFromVOXMesh(const char* buffer, int size, const ConvertOptions& options);


    static void ExportVoxToModelAsync(const char* buffer, int size, const ExportOptions& options, std::function<void(ExportResults)> callback);
    static void GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(MeshingResults)> callback);

private:
};
}
