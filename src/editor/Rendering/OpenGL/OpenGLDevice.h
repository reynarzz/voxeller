#pragma once
#include <Rendering/GfxDevice.h>

class OpenGLDevice : public GfxDevice
{
	void Initialize() override;
	const DeviceInfo& GetInfo() const override;
	std::shared_ptr<Texture> CreateTexture(const TextureDescriptor* desc) override;
	std::shared_ptr<Shader> CreateShader(const ShaderDescriptor* desc) override;
	std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc) override;

	void SetPipelineData(const PipelineData* data) override;
	void DrawRenderable(const RenderableObject* renderable) override;
	std::weak_ptr<RenderTarget> GetRenderTarget() const override;

	void SetRendererGlobalState(const RendererState& uniforms) override;


	void Begin() override;
	void End() override;

private:
	
};
