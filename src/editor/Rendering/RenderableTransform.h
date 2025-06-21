#pragma once
#include <Unvoxeller/Math/VoxVector.h>
#include <Unvoxeller/Math/VoxMatrix.h>

class RenderableTransform
{
public:
    void SetPosition(const Unvoxeller::vox_vec3& position);
    void SetRotation(const Unvoxeller::vox_vec3& eulerAngles);
    void SetScale(const Unvoxeller::vox_vec3& scale);

    const Unvoxeller::vox_mat4& GetModelM() const;
    
private:
    Unvoxeller::vox_mat4 _positionM = {};
    Unvoxeller::vox_mat4 _rotationM = {};
    Unvoxeller::vox_mat4 _scaleM = {};
    Unvoxeller::vox_mat4 _model = {};
};