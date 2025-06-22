#include "RenderableTransform.h"

void RenderableTransform::SetPosition(const glm::vec3 &position)
{
}

void RenderableTransform::SetRotation(const glm::vec3 &eulerAngles)
{
}

void RenderableTransform::SetScale(const glm::vec3 &scale)
{
}

const glm::mat4& RenderableTransform::GetModelM() const
{
    return _model;
}

