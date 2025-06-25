#pragma once
#include <memory>
#include <Rendering/PipelineData.h>
#include <Rendering/PipelineRenderType.h>
#include <unordered_map>
#include <Rendering/GfxDevice.h>
#include <Rendering/ShadersLibrary.h>


class PipelineConfigurations
{
public:
    PipelineConfigurations(GfxDevice* device);
    ~PipelineConfigurations();

    const PipelineData* GetPipelineData(const PipelineRenderType type) const;

private:
    std::shared_ptr<PipelineData> CreateOpaqueUnlitPipeline(GfxDevice* device);
    std::shared_ptr<PipelineData> CreateOpaqueLitPipeline(GfxDevice* device);
    std::shared_ptr<PipelineData> CreateWireFramePipeline(GfxDevice* device);
    std::shared_ptr<PipelineData> CreateTransparentPipeline(GfxDevice* device);



    std::unordered_map<PipelineRenderType, std::shared_ptr<PipelineData>> _pipelinesData = {};
    std::unique_ptr<ShaderLibrary> _shaderLibrary = nullptr;
};