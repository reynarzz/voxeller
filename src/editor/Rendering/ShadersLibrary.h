#pragma once
#include <vector>
#include <unordered_map>
#include <Unvoxeller/Types.h>

// Simple class to obtain shaders, nothing fancy
enum class ShaderType
{
    VERTEX_UNLIT,
    FRAGMENT_UNLIT,
    VERTEX_LIT,
    FRAGMENT_LIT,
    WIRE_GEOMETRY,
    WIRE_FRAGMENT
};

class ShaderLibrary
{
public:
    ShaderLibrary();
    const std::vector<u8>& GetShaderBuffer(const ShaderType type) const;

private:
    std::unordered_map<ShaderType, std::vector<u8>> _shadersBuffers = {};
};