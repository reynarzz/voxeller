#include "PipelineConfigurations.h"

PipelineConfigurations::PipelineConfigurations(GfxDevice* device)
{
    _pipelinesData = 
    {
        { PipelineRenderType::Opaque, CreateOpaquePipeline(device) },
        { PipelineRenderType::Transparent, CreateTransparentPipeline(device) },
    };
}

const std::weak_ptr<PipelineData> PipelineConfigurations::GetPipelineData(const PipelineRenderType type) const
{
    auto it = _pipelinesData.find(type);

    if(it != _pipelinesData.end())
    {
        return it->second;
    }
    
    return {};
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateOpaquePipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    // TODO: 
    ShaderDescriptor desc{};
    //desc.Vertex.resize();


    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateTransparentPipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();
    pipelineData->ZWrite = false;
    pipelineData->Blending = true;



    return pipelineData;
}

PipelineConfigurations::~PipelineConfigurations()
{

}
