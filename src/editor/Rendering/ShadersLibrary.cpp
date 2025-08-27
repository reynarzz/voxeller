#include "ShadersLibrary.h"
#include <Unvoxeller/Log/Log.h>

static const std::string vertexUnlit = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 _MVP_;
uniform mat4 _MODEL_;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = _MVP_ * vec4(aPosition, 1.0);
}
)";

static const std::string VertexLit = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 _MVP_;
uniform mat4 _MODEL_;

flat out vec3 vNormalWorld;

void main() {
    vTexCoord = aTexCoord;
    gl_Position = _MVP_ * vec4(aPosition, 1.0);

     // Transform normal to world-space; assume uModel has no non-uniform scale or handle via normalMatrix
    mat3 normalMatrix = transpose(inverse(mat3(_MODEL_)));
    vNormalWorld = normalMatrix * aNormal;
}
)";

static const std::string fragUnlit = R"(
#version 330 core

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    fragColor = texture(uTexture, vec2(vTexCoord.x, 1.0f - vTexCoord.y));
}
)";

static const std::string fragNoTex = R"(
#version 330 core

out vec4 fragColor;

uniform sampler2D uTexture;

void main() {
    fragColor = vec4(1.0);
}
)";

static const std::string wireFrag = R"(
#version 330 core

in  vec4 gsColor;
out vec4 fragColor;

void main() 
{
    fragColor = gsColor;
}
)";

std::string wireFrame = R"(
#version 330 core
layout(triangles) in;
layout(line_strip, max_vertices=2) out;

uniform mat4 _MVP_;
//uniform vec4 wireColor;

out vec4 gsColor;

void main()
{
    vec4 wireColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // for each edge of the triangle
    for(int e = 0; e < 3; ++e)
    {
        int i0 = e;
        int i1 = (e+1)%3;

        gsColor = wireColor;
        gl_Position = gl_in[i0].gl_Position;
        EmitVertex();

        gsColor = wireColor;
        gl_Position = gl_in[i1].gl_Position;
        EmitVertex();

        EndPrimitive();
    }
}
)";

const std::string fragmentLit = R"(
// Fragment Shader (flat_fs.glsl)
#version 330 core

flat in vec3 vNormalWorld;

uniform vec3 _LIGHT_DIR_;  
uniform vec3 _LIGHT_COLOR_;
uniform float _LIGHT_INTENSITY_;

uniform vec3 uAmbientColor;

out vec4 FragColor;
in vec2 vTexCoord;
uniform sampler2D uTexture;

void main() 
{
    // Ensure unit length
    vec3 N = normalize(vNormalWorld);
    vec3 L = normalize(_LIGHT_DIR_);

    // Lambertian diffuse
    float diff = max(dot(N, L), 0.0);

    // Combine ambient + diffuse
    vec3 color = /*uAmbientColor + */_LIGHT_COLOR_ * diff * _LIGHT_INTENSITY_;
   
    FragColor = texture(uTexture, vec2(vTexCoord.x, 1.0f - vTexCoord.y)) * vec4(color, 1.0);
}
)";

ShaderLibrary::ShaderLibrary()
{
    auto* v0 = reinterpret_cast<const u8*>(vertexUnlit.c_str());
    auto* f0 = reinterpret_cast<const u8*>(fragUnlit.c_str());

    auto* vl0 = reinterpret_cast<const u8*>(VertexLit.c_str());
    auto* fl0 = reinterpret_cast<const u8*>(fragmentLit.c_str());

    auto* g0 = reinterpret_cast<const u8*>(wireFrame.c_str());
    auto* fg0 = reinterpret_cast<const u8*>(wireFrag.c_str());

    auto* ntfg0 = reinterpret_cast<const u8*>(fragNoTex.c_str());


    _shadersBuffers =
    {
       { ShaderType::VERTEX_UNLIT, std::vector<u8>(v0, v0 + vertexUnlit.size() + 1) },
       { ShaderType::FRAGMENT_UNLIT, std::vector<u8>(f0, f0 + fragUnlit.size() + 1) },

       { ShaderType::VERTEX_LIT, std::vector<u8>(vl0, vl0 + VertexLit.size() + 1) },
       { ShaderType::FRAGMENT_LIT, std::vector<u8>(fl0, fl0 + fragmentLit.size() + 1) },

       { ShaderType::WIRE_GEOMETRY, std::vector<u8>(g0, g0 + wireFrame.size() + 1) },
       { ShaderType::WIRE_FRAGMENT, std::vector<u8>(fg0, fg0 + wireFrag.size() + 1) },

       { ShaderType::NO_TEXTURE_FRAGMENT, std::vector<u8>(ntfg0, ntfg0 + fragNoTex.size() + 1) },
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
