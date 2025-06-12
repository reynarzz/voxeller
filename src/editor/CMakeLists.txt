project(voxeller_editor LANGUAGES CXX)

# Add GLFW source
add_subdirectory(${CMAKE_SOURCE_DIR}/third/editor/glfw)

file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_executable(voxeller_editor
    ${EDITOR_SRC}
    ${CMAKE_SOURCE_DIR}/third/editor/gl/glad.cpp
)

target_include_directories(voxeller_editor PRIVATE
    ${CMAKE_SOURCE_DIR}/third/editor
)

target_link_libraries(voxeller_editor PRIVATE
    voxeller
    glfw
)

if(WIN32)
    target_link_libraries(voxeller_editor PRIVATE opengl32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(OpenGL_LIBRARY OpenGL REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

    target_link_libraries(voxeller_editor PRIVATE
        ${COCOA_LIBRARY}
        ${OpenGL_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
endif()
