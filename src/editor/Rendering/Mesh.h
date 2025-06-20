#pragma once
#include <Unvoxeller/Types.h>


class Mesh
{
public:
    s32 GetIndexCount() const;
    s32 GetVertexCount() const;

protected:
    s32 VertexCount;
    s32 IndexCount;
};