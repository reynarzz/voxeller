cmake_minimum_required(VERSION 3.14)
project(Unvoxeller_core LANGUAGES CXX)

# -----------------------------------------------------------------------------
# 1) Assimp configuration (as before)
# -----------------------------------------------------------------------------
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS       OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES     OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_EXPORTERS   ON  CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT         OFF CACHE BOOL "" FORCE)

# compute project root from this CMakeLists location
get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY)

# pull in Assimp
add_subdirectory(
  ${ROOT_DIR}/third/shared/assimp
  ${CMAKE_BINARY_DIR}/third_party/assimp
)

# pull in spdlog
add_subdirectory(
  ${ROOT_DIR}/third/shared/spdlog
  ${CMAKE_BINARY_DIR}/third_party/spdlog
)

# -----------------------------------------------------------------------------
# 2) Add OpenMesh
# -----------------------------------------------------------------------------
# assumes you've cloned OpenMesh into third/shared/OpenMesh
add_subdirectory(
  ${ROOT_DIR}/third/shared/OpenMesh
  ${CMAKE_BINARY_DIR}/third_party/openmesh
)

# -----------------------------------------------------------------------------
# 3) Collect your sources & create your library
# -----------------------------------------------------------------------------
file(GLOB_RECURSE UNVOXELLER_SRC
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)



# optional: organize your IDE/project view
foreach(FILE ${UNVOXELLER_SRC})
  file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}" "${FILE}")
  get_filename_component(FOLDER "${REL_PATH}" PATH)
  source_group("${FOLDER}" FILES "${FILE}")
endforeach()


add_library(Unvoxeller SHARED ${UNVOXELLER_SRC})

 set_target_properties(Unvoxeller PROPERTIES
        OUTPUT_NAME "Unvoxeller"
)

target_compile_definitions(Unvoxeller PRIVATE UNVOXELLER_BUILD_DLL UNVOXELLER_API_EXPORT)

# -----------------------------------------------------------------------------
# 4) Include directories
# -----------------------------------------------------------------------------
target_include_directories(Unvoxeller PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  ${ROOT_DIR}/third/shared          # so we pick up both assimp/, spdlog/ and OpenMesh includes
)

# -----------------------------------------------------------------------------
# 5) Link libraries
# -----------------------------------------------------------------------------
target_link_libraries(Unvoxeller PRIVATE
  assimp
  spdlog

  # OpenMesh core & tools
  OpenMeshCore
  OpenMeshTools
)
