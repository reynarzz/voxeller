#pragma once
#include <Rendering/Shader.h>
#include <Unvoxeller/Types.h>
#include <Rendering/ShaderDescriptor.h>


class GLShader : public Shader
{
public:
    struct ShaderUniformLocations
    {
        s32 MVPLoc = 0;
        s32 MODELLoc = 0;
        s32 lightDirLoc = 0;
        s32 lightColorLoc = 0;
        s32 shadowColorLoc = 0;
    }; 

    GLShader(const ShaderDescriptor* desc);
    ~GLShader();

    GLShader& operator=(const GLShader&) = delete;
    GLShader(const GLShader&) = delete;

    const ShaderUniformLocations& GetUniformLocations() const;
    
    u32 GetID() const;
    void Bind() const;
    void Unbind() const;

private:

    bool CompileShader(const char* shaderCode, u32 shaderType,  u32& shaderId);

    ShaderUniformLocations _locations = {};
    u32 _id = 0;
};