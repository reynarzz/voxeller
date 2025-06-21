#pragma once
#include <memory>

#include <Rendering/GfxDevice.h>
#include <Rendering/PipelineConfigurations.h>
#include <vector>
#include <Rendering/Mesh.h>

class RenderingSystem
{
public:
    RenderingSystem();
    ~RenderingSystem();

    void Initialize();
    void Update();

    void PushMesh(Mesh* mesh);
    
    static std::weak_ptr<GfxDevice> GetDevice();

private:
    static std::shared_ptr<GfxDevice> _device;
    std::unique_ptr<PipelineConfigurations> _pipelinesConfigs = nullptr;
    std::vector<Mesh*> _meshes = {};
    std::vector<std::pair<s32, Mesh*>> _meshesToDestroy = {};
};