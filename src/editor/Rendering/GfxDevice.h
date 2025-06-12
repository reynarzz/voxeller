#pragma once
#include <string>

namespace VoxellerEditor
{
struct MeshDescriptor
{
    
};

struct TextureDescriptor
{
    
};

struct ShaderDescriptor
{
    std::string vertexBuffer;
    std::string fragmentBuffer;
};

struct DeviceInfo
{
    std::string Name;
    
};

// Base class for devices
class GfxDevice
{
public:
    virtual void Initialize() = 0;
    virtual const DeviceInfo& GetInfo() const = 0;
    virtual void* CreateTexture(const TextureDescriptor* desc) = 0;
    virtual void* CreateShader(const ShaderDescriptor* desc) = 0;
    virtual void* CreateMesh(const MeshDescriptor* desc) = 0;
    
    virtual void UpdateMesh(const void** res) = 0;
    virtual void UpdateTexture(const void** res) = 0;
    
    virtual bool DestroyShader(void*) = 0;
    virtual bool DestroyTexture(void*) = 0;
    virtual bool DestroyMesh(void*) = 0;
private:
    
};
}
