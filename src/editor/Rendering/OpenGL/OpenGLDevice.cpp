#include "OpenGLDevice.h"
#include "GLInclude.h"
#include "GLTexture.h"
#include "GLMesh.h"
#include "GLShader.h"

void OpenGLDevice::Initialize()
{

}

const DeviceInfo& OpenGLDevice::GetInfo() const
{
	return {};
}

std::shared_ptr<Texture> OpenGLDevice::CreateTexture(const TextureDescriptor* desc)
{
	return std::make_shared<GLTexture>(desc);
}

std::shared_ptr<Shader> OpenGLDevice::CreateShader(const ShaderDescriptor* desc)
{
	return std::make_shared<GLShader>(desc);
}

std::shared_ptr<Mesh> OpenGLDevice::CreateMesh(const MeshDescriptor* desc)
{
	return std::make_shared<GLMesh>(desc);
}

void OpenGLDevice::DrawMesh(const Mesh *mesh)
{
	const GLMesh* glMesh = static_cast<const GLMesh*>(mesh);
	glMesh->Bind();
	
	// Bind pipeline

	// Draw
	glDrawElements(GL_TRIANGLES, glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	
}

void OpenGLDevice::SetPipelineData(const PipelineData *data)
{
	if(data->ZWrite)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}
	
	if(data->Blending)
	{
		glEnable(GL_BLEND);

		// We are going to support just one type of blending.
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}