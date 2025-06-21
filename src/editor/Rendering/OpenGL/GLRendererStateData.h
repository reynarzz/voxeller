#pragma once
#include <Unvoxeller/Types.h>
#include <Rendering/RendererState.h>

struct GLGlobalUniforms : public RendererState
{
    u32 CameraViewLocation;
};