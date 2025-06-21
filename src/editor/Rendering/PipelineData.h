#pragma once
#include <Rendering/Shader.h>
#include <memory>

struct PipelineData
{
    bool ZWrite = true;
	bool Blending = false;
    std::shared_ptr<Shader> Shader;
};