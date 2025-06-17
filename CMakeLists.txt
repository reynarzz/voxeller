cmake_minimum_required(VERSION 3.14)
project(Unvoxeller_project LANGUAGES CXX)

option(BUILD_VOXELLER_SHARED "Build Unvoxeller as shared library" ON)
option(BUILD_EDITOR "Build Editor" ON)


set(CMAKE_CXX_STANDARD 17)

# Set global output dir
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})


# Add engine/library and editor
add_subdirectory(src/Unvoxeller)

if (BUILD_EDITOR)
  add_subdirectory(src/Editor)
endif()