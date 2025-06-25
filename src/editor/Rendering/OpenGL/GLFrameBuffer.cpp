#include "GLFrameBuffer.h"
#include <Rendering/OpenGL/GLInclude.h>
#include <Unvoxeller/Log/Log.h>


GLFrameBuffer::GLFrameBuffer(const RenderTargetDescriptor* desc)
    :  _fbo(0), _colorTex(0), _depthRbo(0)
{
    _width = (desc->texDescriptor.width);
    _height = (desc->texDescriptor.height);
    _samples = desc->samples;


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

  if (_samples > 1)
  {
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _colorTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, _samples, GL_RGBA8, _width, _height, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, _samples, GL_DEPTH24_STENCIL8, _width, _height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
  else
  {
    // Update color texture
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    
    glBindTexture(GL_TEXTURE_2D, 0);

    // Update depth+stencil renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
}

u32 GLFrameBuffer::GetColorTexture() const 
{
    return _colorTex;
}

u32 GLFrameBuffer::GetFrameBufferID() const
{
    return _fbo;
}

void GLFrameBuffer::Init()
{
  // 1) Color attachment
  if (_samples > 1)
  {
    glGenTextures(1, &_colorTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _colorTex);
    glTexImage2DMultisample(
      GL_TEXTURE_2D_MULTISAMPLE,
      _samples,
      GL_RGBA8,
      _width, _height,
      GL_TRUE                // fixed‐sample locations
    );
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  }
  else
  {
    glGenTextures(1, &_colorTex);
    glBindTexture(GL_TEXTURE_2D, _colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,   GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,   GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  // 2) Depth+Stencil
  glGenRenderbuffers(1, &_depthRbo);
  glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
  if (_samples > 1)
  {
    glRenderbufferStorageMultisample(
      GL_RENDERBUFFER,
      _samples,
      GL_DEPTH24_STENCIL8,
      _width, _height
    );
  }
  else
  {
    glRenderbufferStorage(
      GL_RENDERBUFFER,
      GL_DEPTH24_STENCIL8,
      _width, _height
    );
  }
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  // 3) FBO setup
  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

  if (_samples > 1)
  {
    glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D_MULTISAMPLE,
      _colorTex, 0
    );
  }
  else
  {
    glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D,
      _colorTex, 0
    );
  }

  glFramebufferRenderbuffer(
    GL_FRAMEBUFFER,
    GL_DEPTH_STENCIL_ATTACHMENT,
    GL_RENDERBUFFER,
    _depthRbo
  );

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    LOG_ERROR("GLFrameBuffer incomplete");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLFrameBuffer::~GLFrameBuffer() 
{
    if (_depthRbo) glDeleteRenderbuffers(1, &_depthRbo);
    if (_colorTex) glDeleteTextures(1, &_colorTex);
    if (_fbo)      glDeleteFramebuffers(1, &_fbo);
}
