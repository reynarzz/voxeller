#include "RenderingSystem.h"

// For now force openGL

#include <Rendering/OpenGL/OpenGLDevice.h>

std::shared_ptr<GfxDevice> RenderingSystem::_device = nullptr;

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
    _meshesToDestroy.reserve(_meshes.size());

    for (size_t i = 0; i < _meshes.size(); i++)
    {
       const auto mesh = _meshes[i];

        if(mesh->ShouldDestroy())
        {
            _meshesToDestroy.push_back({i, mesh});
        }
    }
    
    std::sort(_meshesToDestroy.rbegin(), _meshesToDestroy.rend());
    
    for (const auto& toDestroy : _meshesToDestroy)
    {
        std::swap(_meshes[toDestroy.first], _meshes.back());
        _meshes.pop_back();
    }

    for (size_t i = 0; i < _meshes.size(); i++)
    {
       const Mesh* mesh = _meshes[i];
       const PipelineData* data = _pipelinesConfigs->GetPipelineData(mesh->GetPipeline());

       _device->SetPipelineData(data);
       _device->DrawMesh(mesh);
    }
}

void RenderingSystem::PushMesh(Mesh *mesh)
{
    _meshes.push_back(mesh);
}


std::weak_ptr<GfxDevice> RenderingSystem::GetDevice()
{
    return _device;
}

RenderingSystem::~RenderingSystem()
{

}