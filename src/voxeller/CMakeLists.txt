project(voxeller_core LANGUAGES CXX)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_EXPORTERS ON CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT OFF CACHE BOOL "" FORCE)

# Add Assimp source
get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY) # go from src/voxeller to project root


add_subdirectory(
    ${ROOT_DIR}/third/shared/assimp
    ${CMAKE_BINARY_DIR}/third_party/assimp
)
file(GLOB_RECURSE VOXELLER_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(voxeller SHARED
    ${VOXELLER_SRC}
)

target_include_directories(voxeller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
    ${ROOT_DIR}/third/shared
)

target_link_libraries(voxeller PRIVATE
    assimp
)
