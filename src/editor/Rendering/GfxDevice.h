#pragma once
#include <string>

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


struct ShaderObject
{

};

struct PilelineData
{
	bool ZWrite = true;
	bool Bleding = false;
	ShaderObject Shader;

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
	// virtual void SetPipelineData(const PilelineData* data) = 0;
	// virtual void DrawMesh(const void* mesh) = 0;
private:

};
