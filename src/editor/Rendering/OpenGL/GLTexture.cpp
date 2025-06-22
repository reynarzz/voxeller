#include "GLTexture.h"
#include "GLInclude.h"

GLTexture::GLTexture(const TextureDescriptor* desc)
{
    Width = desc->width;
    Height = desc->height;
    
    GL_CALL(glGenTextures(1, &_id));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, _id));

    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, desc->width, desc->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, desc->image));
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if(desc->GenMipMaps)
    {
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));
    }
    else
    {
        GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    }

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    if(desc->GenMipMaps)
    {
        GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
    }

    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}

GLTexture::~GLTexture()
{
    GL_CALL(glDeleteTextures(1, &_id));
}

u32 GLTexture::GetID() const
{
    return _id;
}

void GLTexture::Bind(s32 index) const
{
    GL_CALL(glActiveTexture(GL_TEXTURE0 + index));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, _id));
}
