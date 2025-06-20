#include "RenderingSystem.h"

// For now force openlg
#include <Rendering/OpenGL/OpenGLDevice.h>

std::shared_ptr<GfxDevice> RenderingSystem::_device = nullptr;

RenderingSystem::RenderingSystem()
{
    _device = std::make_shared<OpenGLDevice>();
}

void RenderingSystem::Initialize()
{
    _device->Initialize();
}

void RenderingSystem::Update()
{
    
}

std::weak_ptr<GfxDevice> RenderingSystem::GetDevice()
{
    return _device;
}

RenderingSystem::~RenderingSystem()
{

}