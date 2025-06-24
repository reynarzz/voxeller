#pragma once

// No complex materials systems, just pre-defined draw types.
enum class PipelineRenderType
{
    Opaque_Unlit,
    Opaque_Lit,
    Transparent,
};