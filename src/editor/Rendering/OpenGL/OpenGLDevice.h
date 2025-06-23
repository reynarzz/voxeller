#pragma once
#include <Rendering/GfxDevice.h>

class OpenGLDevice : public GfxDevice
{
	void Initialize() override;
	const DeviceInfo& GetInfo() const override;
	std::shared_ptr<Texture> CreateTexture(const TextureDescriptor* desc) override;
	std::shared_ptr<Shader> CreateShader(const ShaderDescriptor* desc) override;
	std::shared_ptr<Mesh> CreateMesh(const MeshDescriptor* desc) override;
	std::shared_ptr<RenderTarget> CreateRenderTarget(const RenderTargetDescriptor* desc) override;
	void CopyRenderTargetTo(const RenderTarget* from, RenderTarget* to) override;

	void SetPipelineData(const PipelineData* data, const RendererState &uniforms, const RenderableObject *renderable) override;
	void DrawRenderable(const RenderableObject* renderable) override;
	void SetCurrentRenderTarget(const RenderTarget* target) override;
	
	void Begin(const RendererState &uniforms) override;
	void End() override;

private:
	DeviceInfo _deviceInfo = {};
};
