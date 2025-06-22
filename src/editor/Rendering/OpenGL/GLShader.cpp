#include "GLShader.h"
#include "GLInclude.h"
#include <Unvoxeller/Log/Log.h>

GLShader::GLShader(const ShaderDescriptor* desc)
{
    u32 vertId{};
    u32 fragId{};

    if(!CompileShader(reinterpret_cast<const c8*>(desc->Vertex.data()), GL_VERTEX_SHADER, vertId))
        return;

    if(!CompileShader(reinterpret_cast<const c8*>(desc->Fragment.data()), GL_FRAGMENT_SHADER, fragId))
        return;

    _id = glCreateProgram();

    glAttachShader(_id, vertId);
    glAttachShader(_id, fragId);

    glLinkProgram(_id);

    GLint ok = 0;
    glGetProgramiv(_id, GL_LINK_STATUS, &ok);
    if (!ok) 
    {
        GLint len = 0;
        glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, ' ');
        glGetProgramInfoLog(_id, len, &len, &log[0]);

        LOG_ERROR("Program link error: {0}", log);
    }

    glDetachShader(_id, vertId);
    glDetachShader(_id, fragId);
    glDeleteShader(vertId);
    glDeleteShader(fragId);
}

u32 GLShader::GetID() const
{
    return _id;
}

void GLShader::Bind() const
{
    glUseProgram(_id);
}

void GLShader::Unbind() const
{
    glUseProgram(0);
}

bool GLShader::CompileShader(const char* shaderCode, u32 shaderType,  u32& shaderId)
{
    if(shaderCode == nullptr)
    {
        LOG_ERROR(std::string(shaderType == GL_VERTEX_SHADER ? "Vertex" : "Fragment") + " shader code is null.");
        return false;
    }

    shaderId = glCreateShader(shaderType);

    const char* code[] = { shaderCode };

    glShaderSource(shaderId, 1, code, nullptr);

    glCompileShader(shaderId);
    
    GLint ok = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &ok);
    if (!ok) 
    {
        GLint len = 0;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, ' ');
        glGetShaderInfoLog(shaderId, len, &len, &log[0]);
        
        LOG_ERROR(std::string(shaderType == GL_VERTEX_SHADER ? "Vertex" : "Fragment") + " shader compilation failed: {0}", log);
        
        glDeleteShader(shaderId);

        shaderId = 0;

        return false;
    }

    return true;
}

GLShader::~GLShader()
{
    glDeleteProgram(_id);
}