#pragma once
#include <Unvoxeller/Types.h> 
#include <Rendering/RenderTarget.h>
#include <Rendering/RenderTargetDescriptor.h>

class GLFrameBuffer : public RenderTarget
{
public:
    GLFrameBuffer(const RenderTargetDescriptor* desc);
    ~GLFrameBuffer();

    // Bind this FBO for rendering
    void Bind() const;
    // Unbind (bind default framebuffer)
    static void Unbind();

    // Resize color and depth attachments
    void Resize(s32 width, s32 height);

    // Get the color texture ID for sampling
    u32 GetColorTexture() const;
    u32 GetFrameBufferID() const;
    
private:
    void Init();
    u32 _fbo;
    u32 _colorTex;
    u32 _depthRbo;
    s32 _samples = 1;
};