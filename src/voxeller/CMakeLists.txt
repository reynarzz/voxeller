project(voxeller_core LANGUAGES CXX)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)

# Add Assimp source
add_subdirectory(${CMAKE_SOURCE_DIR}/third/shared/assimp)

file(GLOB_RECURSE VOXELLER_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(voxeller SHARED
    ${VOXELLER_SRC}
)

target_include_directories(voxeller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(voxeller PRIVATE
    assimp
)
