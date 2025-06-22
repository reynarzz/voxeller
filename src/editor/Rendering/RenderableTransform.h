#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
class RenderableTransform
{
public:
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& eulerAngles);
    void SetScale(const glm::vec3& scale);

    const glm::mat4& GetModelM() const;
    
private:
    glm::mat4 _positionM = glm::mat4(1.0f);
    glm::mat4 _rotationM = glm::mat4(1.0f);
    glm::mat4 _scaleM = glm::mat4(1.0f);
    glm::mat4 _model = glm::mat4(1.0f);
};