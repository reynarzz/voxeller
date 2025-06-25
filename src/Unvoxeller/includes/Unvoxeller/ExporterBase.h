#pragma once
#include <memory>
#include <vector>
#include <Unvoxeller/Data/ExportOptions.h>
#include <Unvoxeller/Data/UnvoxScene.h>

namespace Unvoxeller
{
   class ExporterBase
   {
   public:
      virtual bool Export(const ExportOptions& options, const std::vector<std::shared_ptr<UnvoxScene>>& scenes) = 0;
   };
}