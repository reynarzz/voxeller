#include "OpenGLDevice.h"

namespace VoxellerEditor
{

    void OpenGLDevice::Initialize() 
    {

    }
    
    const DeviceInfo &OpenGLDevice::GetInfo() const
    {
        return {};
    }
    void *OpenGLDevice::CreateTexture(const TextureDescriptor *desc)
    {
        return nullptr;
    }
    void *OpenGLDevice::CreateShader(const ShaderDescriptor *desc)
    {
        return nullptr;
    }
    void *OpenGLDevice::CreateMesh(const MeshDescriptor *desc)
    {
        return nullptr;
    }

    void OpenGLDevice::UpdateMesh(const void **res)
    {
    }

    void OpenGLDevice::UpdateTexture(const void **res)
    {
    }

    bool OpenGLDevice::DestroyShader(void *)
    {
        return false;
    }

    bool OpenGLDevice::DestroyTexture(void *)
    {
        return false;
    }

    bool OpenGLDevice::DestroyMesh(void *)
    {
        return false;
    }

}