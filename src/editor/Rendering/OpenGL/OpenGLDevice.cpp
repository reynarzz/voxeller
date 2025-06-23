#include "OpenGLDevice.h"
#include "GLInclude.h"
#include "GLTexture.h"
#include "GLMesh.h"
#include "GLShader.h"
#include "GLFrameBuffer.h"

void OpenGLDevice::Initialize() 
{
	_deviceInfo = {};
	
	glGetIntegerv(GL_MAX_SAMPLES, &_deviceInfo.MaxSamples);
	_deviceInfo.Vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));              // e.g. “NVIDIA Corporation”
	_deviceInfo.Renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));            // e.g. “GeForce RTX 3080/PCIe/SSE2”
	_deviceInfo.Version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));             // e.g. “4.6.0 NVIDIA 525.60.13”
	_deviceInfo.ShaderVersion  = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)); // e.g. “4.60 NVIDIA”
}

const DeviceInfo& OpenGLDevice::GetInfo() const
{
	return _deviceInfo;
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
	
	// Bind textures
	const GLTexture* texture = static_cast<GLTexture*>(renderable->GetTexture().lock().get());
	if(texture != nullptr)
	{
		texture->Bind(0);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	// Draw
	glDrawElements(GL_TRIANGLES, glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);

	glMesh->Unbind();
}

// Mimics modern graphics apis behaviour
void OpenGLDevice::SetPipelineData(const PipelineData* data, const RendererState &uniforms, const RenderableObject *renderable)
{
	if(NeedChangePipeline(data))
	{
		// Bind shader
		auto shader = static_cast<GLShader*>(data->Shader.get());
		shader->Bind();

    	glm::mat4 mvp = uniforms.ProjectionViewMatrix * renderable->GetTransform().GetModelM();
		glUniformMatrix4fv(shader->GetUniformLocations().MVPLoc, 1, false, &mvp[0][0]);
		
		GfxDevice::SetPipelineData(data, uniforms, renderable);

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

void OpenGLDevice::Begin(const RendererState &uniforms)
{
	// Clear
	glClearColor(uniforms.Color.x, uniforms.Color.y, uniforms.Color.z, uniforms.Color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void OpenGLDevice::End()
{
	// unbind frame buffer
}

std::shared_ptr<RenderTarget> OpenGLDevice::CreateRenderTarget(const RenderTargetDescriptor * desc)
{
	return std::make_shared<GLFrameBuffer>(desc);
}

void OpenGLDevice::SetCurrentRenderTarget(const RenderTarget* target)
{
	if(target != nullptr)
	{
		static_cast<const GLFrameBuffer*>(target)->Bind();
	}
	else
	{
   		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glViewport();
	}
}

void OpenGLDevice::CopyRenderTargetTo(const RenderTarget* from, RenderTarget* to) 
{
	auto glFrom = static_cast<const GLFrameBuffer*>(from);
	auto glTo = static_cast<const GLFrameBuffer*>(to);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, glFrom->GetFrameBufferID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glTo->GetFrameBufferID());
	glBlitFramebuffer(
		0, 0, glFrom->GetWidth(), glFrom->GetHeight(),
		0, 0, glTo->GetWidth(), glTo->GetHeight(),
		GL_COLOR_BUFFER_BIT, GL_NEAREST
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
