#include "OpenGLDevice.h"
#include "GLInclude.h"
#include "GLTexture.h"
#include "GLMesh.h"
#include "GLShader.h"

void OpenGLDevice::Initialize()
{
	// Create screen framebuffer
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

void OpenGLDevice::DrawRenderable(const RenderableObject *renderable)
{
	const GLMesh* glMesh = static_cast<const GLMesh*>(renderable->GetMesh().lock().get());
	glMesh->Bind();
	
	// Bind pipeline

	// Draw
	glDrawElements(GL_TRIANGLES, glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
}

void OpenGLDevice::SetPipelineData(const PipelineData* data)
{
	if(NeedChangePipeline(data))
	{
		GfxDevice::SetPipelineData(data);

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
}

void OpenGLDevice::Begin()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLDevice::End()
{

}

std::weak_ptr<RenderTarget> OpenGLDevice::GetRenderTarget() const 
{
    return _renderTarget;
}
