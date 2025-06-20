#include "OpenGLDevice.h"
#include "GLTexture.h"


void OpenGLDevice::Initialize()
{

}

const DeviceInfo& OpenGLDevice::GetInfo() const
{
	return {};
}

std::shared_ptr<Texture> OpenGLDevice::CreateTexture(const TextureDescriptor* desc)
{
	return std::static_pointer_cast<Texture>(std::make_shared<GLTexture>(desc));
}

void* OpenGLDevice::CreateShader(const ShaderDescriptor* desc)
{
	return nullptr;
}

void* OpenGLDevice::CreateMesh(const MeshDescriptor* desc)
{
	return nullptr;
}

void OpenGLDevice::UpdateMesh(const void** res)
{
}

void OpenGLDevice::UpdateTexture(const void** res)
{
}

bool OpenGLDevice::DestroyShader(void*)
{
	return false;
}

bool OpenGLDevice::DestroyTexture(void*)
{
	return false;
}

bool OpenGLDevice::DestroyMesh(void*)
{
	return false;
}

