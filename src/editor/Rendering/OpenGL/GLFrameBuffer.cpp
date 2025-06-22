#include "GLFrameBuffer.h"
#include <Rendering/OpenGL/GLInclude.h>
#include <Unvoxeller/Log/Log.h>


GLFrameBuffer::GLFrameBuffer(const RenderTargetDescriptor* desc)
    :  _fbo(0), _colorTex(0), _depthRbo(0)
{
    _width = (desc->texDescriptor.width);
    _height = (desc->texDescriptor.height);

    if (_width <= 0 || _height <= 0)
    {
        LOG_ERROR("Width and height must be positive");
        throw;
    }
    Init();
}

void GLFrameBuffer::Bind() const 
{
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _width, _height);
}

void GLFrameBuffer::Unbind() 
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLFrameBuffer::Resize(int width, int height) 
{
    if (width == _width && height == _height)
        return;
    _width = width;
    _height = height;

    // Update color texture
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Update depth+stencil renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

u32 GLFrameBuffer::GetColorTexture() const 
{
    return _colorTex;
}

void GLFrameBuffer::Init()
{
    // Create color texture
    glGenTextures(1, &_colorTex);
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth+stencil renderbuffer
    glGenRenderbuffers(1, &_depthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Create framebuffer and attach
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthRbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
    {
        LOG_ERROR("GLFrameBuffer is not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLFrameBuffer::~GLFrameBuffer() 
{
    if (_depthRbo) glDeleteRenderbuffers(1, &_depthRbo);
    if (_colorTex) glDeleteTextures(1, &_colorTex);
    if (_fbo)      glDeleteFramebuffers(1, &_fbo);
}
