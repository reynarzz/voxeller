#pragma once
#include <vector>
#include <Unvoxeller/Types.h>
#include <Rendering/PipelineRenderType.h>
#include <Rendering/MeshRenderType.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 UV;
};

struct MeshDescriptor
{
    MeshRenderType RenderType;
    std::vector<Vertex> Vertices;
    std::vector<u32> Indices;
};
