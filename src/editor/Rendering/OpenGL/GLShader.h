#pragma once
#include <Rendering/Shader.h>
#include <Unvoxeller/Types.h>
#include <Rendering/ShaderDescriptor.h>


class GLShader : public Shader
{
public:
    GLShader(const ShaderDescriptor* desc);
    ~GLShader();

    GLShader& operator=(const GLShader&) = delete;
    GLShader(const GLShader&) = delete;

    u32 GetID() const;
    void Bind() const;
    void Unbind() const;

private:

    bool CompileShader(const char* shaderCode, u32 shaderType,  u32& shaderId);
    u32 _id = 0;
};