#pragma once
#include <Rendering/GfxDevice.h>
#include <memory>


class RenderingSystem
{
public:
    RenderingSystem();
    ~RenderingSystem();

    void Initialize();
    void Update();

    static std::weak_ptr<GfxDevice> GetDevice();

private:
    static std::shared_ptr<GfxDevice> _device;
};