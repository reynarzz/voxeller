#pragma once
#include <string>
#include <vector>
#include <functional>
#include <Unvoxeller/api.h>
#include <array>
#include <Unvoxeller/Data/ExportResults.h>
#include <Unvoxeller/Data/MemData.h>
#include <Unvoxeller/Data/ConvertResults.h>
#include <Unvoxeller/Data/ConvertOptions.h>
#include <Unvoxeller/Data/ExportOptions.h>

namespace Unvoxeller
{
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
