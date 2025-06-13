#pragma once
#include <string>
#include <voxeller/Types.h>
#include <vector>
#include <functional>

namespace Voxeller
{

enum class ModelFormat
{
    FBX, OBJ, 
};

struct MeshInfo
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

struct ConvertResult
{
    ConvertMSG Msg;
    std::vector<MeshInfo> meshes;
};

struct ExportResults
{
    ConvertResult Convert = {};
    std::string OutPath;
};

struct ConvertOptions
{
    // Remove T-Juntions
    bool NoTJunctions = false;

    // Make Textures to always be power of two
    bool TexturesPOT = false;

    // Should every mesh have a separated texture?
    bool SeparateTexturesPerMesh = false;

    // Vertices will be shared across faces.
    bool WeldVertices = false;

    // Hard edges
    bool FlatShading = true;


    bool ExportFramesSeparatelly = true;
};

struct ExportOptions
{
    ConvertOptions ConvertOptions;
    ModelFormat OutputFormat;
};

struct MeshingResults
{
    ConvertResult Convert;
};

class GreedyMesher
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
