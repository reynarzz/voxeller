#pragma once
#include <memory>

#include <Rendering/GfxDevice.h>
#include <Rendering/PipelineConfigurations.h>
#include <vector>
#include <Rendering/Mesh.h>
#include <Rendering/RenderableObject.h>

class RenderingSystem
{
public:
    RenderingSystem();
    ~RenderingSystem();

    void Initialize();
    void Update();

    static void PushRenderable(const RenderableObject* renderable);
    
    const static std::weak_ptr<RenderTarget> GetRenderTarget();
    static std::weak_ptr<GfxDevice> GetDevice();
    
    void SetViewMatrix(void* viewM);

private:
    static std::shared_ptr<GfxDevice> _device;
    static std::vector<const RenderableObject*> _renderables;

    std::unique_ptr<PipelineConfigurations> _pipelinesConfigs = nullptr;
    std::vector<std::pair<s32, const RenderableObject*>> _meshesToDestroy = {};
};