#include "ShadersLibrary.h"
#include <Unvoxeller/Log/Log.h>

static const std::string vertexUnlit = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 _MVP_;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = _MVP_ * vec4(aPosition, 1.0);
}
)";

static const std::string fragUnlit = R"(
#version 330 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";


ShaderLibrary::ShaderLibrary()
{
    auto* v0 = reinterpret_cast<const u8*>(vertexUnlit.c_str());
    auto* f0 = reinterpret_cast<const u8*>(fragUnlit.c_str());

    _shadersBuffers =
    {
       { ShaderType::VERTEX_UNLIT, std::vector<u8>(v0, v0 + vertexUnlit.size() + 1) },
       { ShaderType::FRAGMENT_UNLIT, std::vector<u8>(f0, f0 + fragUnlit.size() + 1) }
    };
}

const std::vector<u8> &ShaderLibrary::GetShaderBuffer(const ShaderType type) const
{
    auto it = _shadersBuffers.find(type);

    if(it != _shadersBuffers.end())
    {
        return it->second;
    }

    LOG_ERROR("Shader type doesn't exists");

    static std::vector<u8> emptyBuffer;

    return emptyBuffer;
}
