#pragma once

// NOTE: forcing to opengl, ideally this would check which API is rendering.
#include <Rendering/OpenGL/GLTexture.h>

#include <Unvoxeller/Types.h>

#define TEXTURE_TO_IMGUI(x) \
        (long*)std::static_pointer_cast<GLTexture>(x)->GetID()