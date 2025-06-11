cmake_minimum_required(VERSION 3.5)

project(voxeller LANGUAGES CXX)

set(EDITOR_PROJECT_NAME "voxeller")
set(CORE_PROJECT_NAME "voxellerd")
set(CORE_ROOT "src/core")
set(EDITOR_ROOT "src/editor")
set(EDITOR_THIRD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third/editor")
set(CORE_THIRD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third/core")
set(SHARED_THIRD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third/shared")


file(GLOB_RECURSE CORE_SRC CONFIGURE_DEPENDS 
${CORE_ROOT}/*.h
${CORE_ROOT}/*.cpp
)

file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS 
${EDITOR_ROOT}/*.h
${EDITOR_ROOT}/*.cpp
)


if(WIN32)
add_definitions(
-DWINDOWS
)


set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_STANDARD 17)
endif()

# Core ------------------------------------------------------
add_library(${CORE_PROJECT_NAME} SHARED
${CORE_SRC}
)

target_include_directories(${CORE_PROJECT_NAME} PRIVATE
${SHARED_THIRD_ROOT}
${CORE_ROOT}/includes
${CORE_THIRD_ROOT}/assimp/include
)

target_link_libraries(${CORE_PROJECT_NAME} PRIVATE
${CORE_THIRD_ROOT}/assimp_vs/lib/assimpd.lib
${CORE_THIRD_ROOT}/assimp_vs/lib/zlibstaticd.lib
)

# Editor-----------------------------------------------------
add_executable(${EDITOR_PROJECT_NAME}
${EDITOR_SRC}
${EDITOR_THIRD_ROOT}/gl/glad.cpp
)

target_include_directories(${EDITOR_PROJECT_NAME} PRIVATE
${EDITOR_THIRD_ROOT}
${EDITOR_THIRD_ROOT}/glfw/include
${SHARED_THIRD_ROOT}
${CORE_ROOT}/includes #Adds core includes
)

target_link_libraries(${EDITOR_PROJECT_NAME} PRIVATE
${CORE_PROJECT_NAME}
${EDITOR_THIRD_ROOT}/glfw/src/glfw3.lib
opengl32.lib
)
