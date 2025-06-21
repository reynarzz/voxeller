#include "RenderingSystem.h"

// For now force openGL
#include <Rendering/OpenGL/OpenGLDevice.h>

std::shared_ptr<GfxDevice> RenderingSystem::_device = nullptr;
std::vector<const RenderableObject*> RenderingSystem::_renderables = {};

RenderingSystem::RenderingSystem()
{
    _device = std::make_shared<OpenGLDevice>();
    _pipelinesConfigs = std::make_unique<PipelineConfigurations>(_device.get());
}

void RenderingSystem::Initialize()
{
    _device->Initialize();
}

void RenderingSystem::Update()
{
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

    _device->Begin();

    // Move
	constexpr f32 colorTest = 0.05f;
    RendererState state{};
    state.Color = { colorTest, colorTest, colorTest, 1.0f};
    
    _device->SetRendererGlobalState(state);

    for (size_t i = 0; i < _renderables.size(); i++)
    {
       const RenderableObject* renderable = _renderables[i];
       const PipelineData* data = _pipelinesConfigs->GetPipelineData(renderable->GetRenderType());
       
       _device->SetPipelineData(data);
       // Set uniforms (camera view, etc..)
       
       // Draw
       _device->DrawRenderable(renderable);
    }

    _device->End();
}

void RenderingSystem::PushRenderable(const RenderableObject *renderable)
{
    _renderables.push_back(renderable);
}

const std::weak_ptr<RenderTarget> RenderingSystem::GetRenderTarget()
{
    return _device->GetRenderTarget();
}

std::weak_ptr<GfxDevice> RenderingSystem::GetDevice()
{
    return _device;
}

RenderingSystem::~RenderingSystem()
{

}