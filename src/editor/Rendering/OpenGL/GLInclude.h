#pragma once

#include <gl/glad.h>
#include <iostream>

#ifndef GL_CALL(...)
#define GL_CALL(x) \
    do { \
        x; \
        GLenum err; \
        while ((err = glGetError()) != GL_NO_ERROR) \
            std::cerr << "OpenGL Error 0x" << std::hex << err \
                      << " at " << __FILE__ << ":" << __LINE__ \
                      << std::dec << std::endl; \
    } while (0)
#endif