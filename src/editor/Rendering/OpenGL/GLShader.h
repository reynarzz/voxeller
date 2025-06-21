#pragma once
#include <Rendering/Shader.h>
#include <Unvoxeller/Types.h>
#include <Rendering/ShaderDescriptor.h>


class GLShader : public Shader
{
public:
    GLShader(const ShaderDescriptor* desc);
    ~GLShader();
    u32 GetID();
    void Bind();
    void Unbind();

private:
    u32 _id;
};