#pragma once
#include <Rendering/GfxDevice.h>

namespace VoxellerEditor
{
    class OpenGLDevice : public GfxDevice
    {
      void Initialize() override;
     const DeviceInfo& GetInfo() const override;
     void* CreateTexture(const TextureDescriptor* desc) override;
     void* CreateShader(const ShaderDescriptor* desc) override;
     void* CreateMesh(const MeshDescriptor* desc) override;
    
     void UpdateMesh(const void** res) override;
     void UpdateTexture(const void** res) override;
    
     bool DestroyShader(void*) override;
     bool DestroyTexture(void*) override;
     bool DestroyMesh(void*) override;
    };
}
