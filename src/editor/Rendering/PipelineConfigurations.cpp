#include "PipelineConfigurations.h"

PipelineConfigurations::PipelineConfigurations(GfxDevice* device)
{
    _shaderLibrary = std::make_unique<ShaderLibrary>();

    _pipelinesData = 
    {
        { PipelineRenderType::Opaque_Unlit, CreateOpaqueUnlitPipeline(device) },
        { PipelineRenderType::Opaque_Lit, CreateOpaqueLitPipeline(device) },
        { PipelineRenderType::Transparent, CreateTransparentPipeline(device) },
        { PipelineRenderType::Wireframe, CreateWireFramePipeline(device) },
        { PipelineRenderType::NoTexture, CreateNoTextureUnlitPipeline(device) },
        
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

std::shared_ptr<PipelineData> PipelineConfigurations::CreateOpaqueUnlitPipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    // TODO: Create the opaque shader
    ShaderDescriptor desc{};
    desc.Vertex = _shaderLibrary->GetShaderBuffer(ShaderType::VERTEX_UNLIT);
    desc.Fragment = _shaderLibrary->GetShaderBuffer(ShaderType::FRAGMENT_UNLIT);
    desc.Geometry = {};

    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateWireFramePipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    ShaderDescriptor desc{};
    desc.Vertex = _shaderLibrary->GetShaderBuffer(ShaderType::VERTEX_UNLIT);
    desc.Fragment = _shaderLibrary->GetShaderBuffer(ShaderType::WIRE_FRAGMENT);
    desc.Geometry = _shaderLibrary->GetShaderBuffer(ShaderType::WIRE_GEOMETRY);

    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}


std::shared_ptr<PipelineData> PipelineConfigurations::CreateOpaqueLitPipeline(GfxDevice *device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    ShaderDescriptor desc{};
    desc.Vertex = _shaderLibrary->GetShaderBuffer(ShaderType::VERTEX_LIT);
    desc.Fragment = _shaderLibrary->GetShaderBuffer(ShaderType::FRAGMENT_LIT);
    desc.Geometry = {};

    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateTransparentPipeline(GfxDevice *device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    // TODO: Transparent

    return pipelineData;
}

std::shared_ptr<PipelineData> PipelineConfigurations::CreateNoTextureUnlitPipeline(GfxDevice* device)
{
    auto pipelineData = std::make_shared<PipelineData>();

    ShaderDescriptor desc{};
    desc.Vertex = _shaderLibrary->GetShaderBuffer(ShaderType::VERTEX_LIT);
    desc.Fragment = _shaderLibrary->GetShaderBuffer(ShaderType::NO_TEXTURE_FRAGMENT);
    desc.Geometry = {};

    pipelineData->ZWrite = true;
    pipelineData->Blending = false;
    pipelineData->Shader = device->CreateShader(&desc);

    return pipelineData;
}

PipelineConfigurations::~PipelineConfigurations()
{

}
