#include "RenderableTransform.h"
#include <glm/gtc/matrix_transform.hpp>

static const glm::mat4 _identity = glm::mat4(1.0f);


void RenderableTransform::SetPosition(const glm::vec3 &position)
{
    _positionM = glm::translate(_identity, position);

    ApplyModel();
}

void RenderableTransform::SetRotation(const glm::vec3 &eulerAngles)
{
    glm::mat4 xRot = glm::rotate(_identity, glm::radians(eulerAngles.x), {1, 0, 0});
    glm::mat4 yRot = glm::rotate(_identity, glm::radians(eulerAngles.y), {0, 1, 0});
    glm::mat4 zRot = glm::rotate(_identity, glm::radians(eulerAngles.z), {0, 0, 1});

    _rotationM = zRot * yRot * xRot;

    ApplyModel();

}

void RenderableTransform::SetScale(const glm::vec3 &scale)
{
    _scaleM = glm::scale(_identity, scale);

    ApplyModel();

}

const glm::mat4& RenderableTransform::GetModelM() const
{
    return _model;
}

void RenderableTransform::ApplyModel()
{
    _model = _positionM * _rotationM * _scaleM;
}
