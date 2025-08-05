#pragma once
#include <Unvoxeller/ExporterBase.h>

namespace Unvoxeller
{
	class AssimpSceneWritter : public ExporterBase
	{
	public:
		bool Export(const ExportOptions& options, const ConvertOptions& cOptions, 
				    const std::vector<std::shared_ptr<UnvoxScene>>& scenes) override;
	private:
	};
}