cmake_minimum_required(VERSION 3.14)
project(voxeller_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Add engine/library and editor
add_subdirectory(src/voxeller)
add_subdirectory(src/editor)
