#include "RenderingSystem.h"

// For now force openGL
#include <Rendering/OpenGL/OpenGLDevice.h>
#include <Unvoxeller/Log/Log.h>

std::shared_ptr<GfxDevice> RenderingSystem::_device = nullptr;
std::vector<const RenderableObject*> RenderingSystem::_renderables = {};
std::shared_ptr<RenderTarget> RenderingSystem::_renderTarget = nullptr;
std::shared_ptr<RenderTarget> RenderingSystem::_AArenderTarget = nullptr;

RenderingSystem::RenderingSystem()
{
    _device = std::make_shared<OpenGLDevice>();
    _pipelinesConfigs = std::make_unique<PipelineConfigurations>(_device.get());
}

void RenderingSystem::Initialize()
{
    _device->Initialize();

    RenderTargetDescriptor rDesc{};
    rDesc.texDescriptor.width = 1;
    rDesc.texDescriptor.height = 1;
    rDesc.samples = 1;

    _renderTarget = _device->CreateRenderTarget(&rDesc);

    RenderTargetDescriptor rDescAA{};
    rDescAA.texDescriptor.width = 1;
    rDescAA.texDescriptor.height = 1;
    rDescAA.samples = glm::min(6, _device->GetInfo().MaxSamples);
    
    LOG_INFO("Max Samples: {0}", rDescAA.samples);
    LOG_INFO("Vendor: {0}", _device->GetInfo().Vendor);
    LOG_INFO("Renderer: {0}", _device->GetInfo().Renderer);
    LOG_INFO("Shader ver: {0}", _device->GetInfo().ShaderVersion);

    _AArenderTarget = _device->CreateRenderTarget(&rDescAA);
}

void RenderingSystem::Update(const RendererState& state)
{
    if(_renderTarget->GetWidth() != state.ScrWidth || _renderTarget->GetHeight() != state.ScrHeight) 
    {
        _renderTarget->Resize(state.ScrWidth, state.ScrHeight);
        _AArenderTarget->Resize(state.ScrWidth, state.ScrHeight);
    }

    _meshesToDestroy.clear();
    _meshesToDestroy.reserve(_renderables.size());
    
    for (size_t i = 0; i < _renderables.size(); i++)
    {
        const auto renderable = _renderables[i];

        if(renderable->ShouldDestroy())
        {
            _meshesToDestroy.push_back({i, renderable});
        }
    }

    std::sort(_meshesToDestroy.rbegin(), _meshesToDestroy.rend());
    
    for (const auto& toDestroy : _meshesToDestroy)
    {
        std::swap(_renderables[toDestroy.first], _renderables.back());
        _renderables.pop_back();
    }

    _device->SetCurrentRenderTarget(_AArenderTarget.get());
    _device->Begin(state);
    
    for (size_t i = 0; i < _renderables.size(); i++)
    {
       const RenderableObject* renderable = _renderables[i];
       const PipelineData* data = _pipelinesConfigs->GetPipelineData(renderable->GetRenderType());
       
       _device->SetPipelineData(data, state, renderable);
       
       // Draw
       _device->DrawRenderable(renderable);
    }

    _device->End();
    _device->SetCurrentRenderTarget(nullptr);

    // Copies AA render target to non AA target
    _device->CopyRenderTargetTo(_AArenderTarget.get(), _renderTarget.get());
}

void RenderingSystem::PushRenderable(const RenderableObject *renderable)
{
    _renderables.push_back(renderable);
}

const std::weak_ptr<RenderTarget> RenderingSystem::GetRenderTarget()
{
    return _renderTarget;
}

std::weak_ptr<GfxDevice> RenderingSystem::GetDevice()
{
    return _device;
}

RenderingSystem::~RenderingSystem()
{

}