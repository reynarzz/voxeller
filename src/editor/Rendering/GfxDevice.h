#pragma once
#include <string>
#include <Rendering/Texture.h>
#include <Rendering/Mesh.h>
#include <Rendering/Shader.h>
#include <Rendering/PipelineData.h>

// Descriptors;
#include <Rendering/MeshDescriptor.h>
#include <Rendering/TextureDescriptor.h>
#include <Rendering/ShaderDescriptor.h>

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
	virtual std::shared_ptr<Texture> CreateTexture(const TextureDescriptor* desc) = 0;
	virtual std::shared_ptr<Shader> CreateShader(const ShaderDescriptor* desc) = 0;
	virtual std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc) = 0;

	virtual void SetPipelineData(const PipelineData* data) = 0;
	virtual void DrawMesh(const Mesh* mesh) = 0;
private:

};