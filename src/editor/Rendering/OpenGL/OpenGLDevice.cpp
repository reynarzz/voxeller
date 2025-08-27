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
	_deviceInfo.Vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));              
	_deviceInfo.Renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));           
	_deviceInfo.Version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));             
	_deviceInfo.ShaderVersion  = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
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
		GL_CALL(glActiveTexture(GL_TEXTURE0 + 0));
		GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
	}
	
	if (renderable->GetDrawType() == RenderDrawType::Triangles)
	{
		// Draw
		GL_CALL(glDrawElements(GL_TRIANGLES, glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr));
	}
	else if (renderable->GetDrawType() == RenderDrawType::Lines)
	{
		GL_CALL(glDrawElements(GL_LINES, glMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr));
	}
	else 
	{
		
	}
	
	glMesh->Unbind();
}

// Mimics modern graphics apis behaviour
void OpenGLDevice::SetPipelineData(const PipelineData* data, const RendererState &uniforms, const RenderableObject *renderable)
{
	if(NeedChangePipeline(data))
	{
		// Bind shader
		const GLShader* shader = static_cast<GLShader*>(data->Shader.get());
		shader->Bind();

    	glm::mat4 mvp = uniforms.ViewState->ProjectionViewMatrix * renderable->GetTransform().GetModelM();
		GL_CALL(glUniformMatrix4fv(shader->GetUniformLocations().MVPLoc, 1, false, &mvp[0][0]));
		GL_CALL(glUniformMatrix4fv(shader->GetUniformLocations().MODELLoc, 1, false, &renderable->GetTransform().GetModelM()[0][0]));
		GL_CALL(glUniform3f(shader->GetUniformLocations().lightDirLoc, uniforms.LightState->lightDir.x, uniforms.LightState->lightDir.y, uniforms.LightState->lightDir.z));
		GL_CALL(glUniform3f(shader->GetUniformLocations().lightColorLoc, uniforms.LightState->lightColor.x, uniforms.LightState->lightColor.y, uniforms.LightState->lightColor.z));
		GL_CALL(glUniform3f(shader->GetUniformLocations().shadowColorLoc, uniforms.LightState->shadowColor.x, uniforms.LightState->shadowColor.y, uniforms.LightState->shadowColor.z));
		GL_CALL(glUniform1f(shader->GetUniformLocations().lightIntensityLoc, uniforms.LightState->lightIntensity));
		
		GfxDevice::SetPipelineData(data, uniforms, renderable);

		if(data->ZWrite)
		{
			GL_CALL(glEnable(GL_DEPTH_TEST));
		}
		else
		{
			GL_CALL(glDisable(GL_DEPTH_TEST));
		}

		if(data->Blending)
		{
			GL_CALL(glEnable(GL_BLEND));

			// We are going to support just one type of blending.
			GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		}
		else
		{
			GL_CALL(glDisable(GL_BLEND));
		}
	}
}

void OpenGLDevice::Begin(const RendererState &uniforms)
{
	// Clear
	glClearColor(uniforms.ViewState->Color.x, uniforms.ViewState->Color.y, uniforms.ViewState->Color.z, uniforms.ViewState->Color.w);
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
		const GLFrameBuffer* fbo = static_cast<const GLFrameBuffer*>(target);
		fbo->Bind();

    	glViewport(0, 0, target->GetWidth(), target->GetHeight());
	}
	else
	{
   		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void OpenGLDevice::CopyRenderTargetTo(const RenderTarget* from, RenderTarget* to) 
{
	const GLFrameBuffer* glFrom = static_cast<const GLFrameBuffer*>(from);
	const GLFrameBuffer* glTo = static_cast<const GLFrameBuffer*>(to);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, glFrom->GetFrameBufferID());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glTo->GetFrameBufferID());
	glBlitFramebuffer(
		0, 0, glFrom->GetWidth(), glFrom->GetHeight(),
		0, 0, glTo->GetWidth(), glTo->GetHeight(),
		GL_COLOR_BUFFER_BIT, GL_NEAREST
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
