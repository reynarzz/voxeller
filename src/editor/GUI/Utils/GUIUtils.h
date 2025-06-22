#pragma once

// NOTE: forcing to opengl, ideally this would check which API is rendering.
#include <Rendering/OpenGL/GLTexture.h>
#include <Rendering/OpenGL/GLFrameBuffer.h>
#include <Unvoxeller/Types.h>

#define TEXTURE_TO_IMGUI(x) \
        (long*)std::static_pointer_cast<GLTexture>(x)->GetID()

#define RENDER_TARGET_TO_IMGUI(x) \
        (long*)static_cast<const GLFrameBuffer*>(x)->GetColorTexture()