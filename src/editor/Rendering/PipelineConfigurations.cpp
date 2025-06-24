#include "PipelineConfigurations.h"

PipelineConfigurations::PipelineConfigurations(GfxDevice* device)
{
    _shaderLibrary = std::make_unique<ShaderLibrary>();

    _pipelinesData = 
    {
        { PipelineRenderType::Opaque, CreateOpaquePipeline(device) },
        { PipelineRenderType::Transparent, CreateTransparentPipeline(device) },
    };
}

const PipelineData* PipelineConfigurations::GetPipelineData(const PipelineRenderType type) const
{
    auto it = _pipelinesData.find(type);

    if(it != _pipelinesData.end())
    {
        return it->second.get();
    }
    
    return nullptr;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateOpaquePipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    // TODO: Create the opaque shader
    ShaderDescriptor desc{};
    desc.Vertex = _shaderLibrary->GetShaderBuffer(ShaderType::VERTEX_LIT);
    desc.Fragment = _shaderLibrary->GetShaderBuffer(ShaderType::FRAGMENT_LIT);
    desc.Geometry = {};
   // desc.Geometry = _shaderLibrary->GetShaderBuffer(ShaderType::WIRE_GEOMETRY);

    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateTransparentPipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    // TODO: Transparent

    return pipelineData;
}

PipelineConfigurations::~PipelineConfigurations()
{

}
