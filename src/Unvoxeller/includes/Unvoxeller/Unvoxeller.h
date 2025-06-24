#pragma once

#include <Unvoxeller/api.h>
#include <Unvoxeller/Data/ExportResults.h>
#include <Unvoxeller/Data/ConvertResults.h>
#include <Unvoxeller/Data/ConvertResults.h>
#include <Unvoxeller/Data/ConvertOptions.h>
#include <Unvoxeller/Data/ExportOptions.h>

#include <string>
#include <vector>
#include <functional>

namespace Unvoxeller
{
	class UNVOXELLER_API Unvoxeller
	{
	public:
		Unvoxeller();
		~Unvoxeller();

		Unvoxeller(const Unvoxeller&) = delete;
		Unvoxeller& operator=(const Unvoxeller&) = delete;
		
		ExportResults ExportVoxToModel(const std::string& inVoxPath, const std::string& outExportPath, const ExportOptions& options);
		ExportResults ExportVoxToModel(const char* buffer, int size, const ExportOptions& options);

		ConvertResult VoxToMem(const std::string& inVoxPath, const ConvertOptions& options);
		ConvertResult VoxToMem(const char* buffer, int size, const ConvertOptions& options);

		void ExportVoxToModelAsync(const char* buffer, int size, const ExportOptions& options, std::function<void(ExportResults)> callback);
		void GetModelFromVOXMeshAsync(const char* buffer, int size, const ConvertOptions& options, std::function<void(ConvertResult)> callback);
	private:
	};
}