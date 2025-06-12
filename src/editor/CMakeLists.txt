project(voxeller_editor LANGUAGES CXX)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY) # go from src/voxeller to project root


# Add GLFW source
add_subdirectory(${ROOT_DIR}/third/editor/glfw
${CMAKE_BINARY_DIR}/third_party/glfw
)

file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_executable(voxeller_editor
    ${EDITOR_SRC}
    ${ROOT_DIR}/third/editor/gl/glad.cpp
)

target_include_directories(voxeller_editor PRIVATE
    ${ROOT_DIR}/third/editor
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
