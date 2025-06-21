#include "RenderableTransform.h"

void RenderableTransform::SetPosition(const Unvoxeller::vox_vec3 &position)
{
}

void RenderableTransform::SetRotation(const Unvoxeller::vox_vec3 &eulerAngles)
{
}

void RenderableTransform::SetScale(const Unvoxeller::vox_vec3 &scale)
{
}

const Unvoxeller::vox_mat4& RenderableTransform::GetModelM() const
{
    return _model;
}

