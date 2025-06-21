#pragma once
#include <Unvoxeller/Math/VoxMatrix.h>

class Camera
{
    public:
        const Unvoxeller::vox_mat4* GetViewMatrix() const;
        void Update();
    private:
};