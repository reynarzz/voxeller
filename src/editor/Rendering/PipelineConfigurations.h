#pragma once
#include <memory>
#include <Rendering/PipelineData.h>
#include <Rendering/PipelineRenderType.h>
#include <unordered_map>
#include <Rendering/GfxDevice.h>

class PipelineConfigurations
{
public:
    PipelineConfigurations(GfxDevice* device);
    ~PipelineConfigurations();

    const PipelineData* GetPipelineData(const PipelineRenderType type) const;

private:

    std::shared_ptr<PipelineData> CreateOpaquePipeline(GfxDevice* device);
    std::shared_ptr<PipelineData> CreateTransparentPipeline(GfxDevice* device);

    std::unordered_map<PipelineRenderType, std::shared_ptr<PipelineData>> _pipelinesData;
};