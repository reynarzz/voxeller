project(Unvoxeller_editor LANGUAGES CXX OBJCXX)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(ROOT_DIR ${ROOT_DIR} DIRECTORY) # go from src/Unvoxeller to project root


# Add GLFW source
add_subdirectory(${ROOT_DIR}/third/editor/glfw
${CMAKE_BINARY_DIR}/third_party/glfw
)

# ImGui source files
set(IMGUI_SRC
    ${ROOT_DIR}/third/editor/imgui/imgui.cpp
    ${ROOT_DIR}/third/editor/imgui/imgui_draw.cpp
    ${ROOT_DIR}/third/editor/imgui/imgui_tables.cpp
    ${ROOT_DIR}/third/editor/imgui/imgui_widgets.cpp
    ${ROOT_DIR}/third/editor/imgui/backends/imgui_impl_glfw.cpp
    ${ROOT_DIR}/third/editor/imgui/backends/imgui_impl_opengl3.cpp
    # Optional demo:
    ${ROOT_DIR}/third/editor/imgui/imgui_demo.cpp
)

add_library(imgui STATIC ${IMGUI_SRC})

target_include_directories(imgui PUBLIC
    ${ROOT_DIR}/third/editor/imgui
    ${ROOT_DIR}/third/editor/imgui/backends
    ${ROOT_DIR}/third/editor/glfw/include
)

file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

foreach(FILE ${EDITOR_SRC})
    # Get relative path from project root
    file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}" "${FILE}")

    # Extract folder path (e.g. src/Unvoxeller/mesh)
    get_filename_component(FOLDER "${REL_PATH}" PATH)

    # Set the source group
    source_group("${FOLDER}" FILES "${FILE}")
endforeach()


if(APPLE)
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/GUI/Utils/DropHover.mm
        PROPERTIES
        LANGUAGE OBJCXX
    )
endif()

add_executable(Unvoxeller_editor
    ${EDITOR_SRC}
    ${ROOT_DIR}/third/editor/gl/glad.cpp
    $<$<BOOL:WIN32>:${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc>
    $<$<BOOL:APPLE>:${CMAKE_CURRENT_SOURCE_DIR}/GUI/Utils/DropHover.mm>
)

  target_compile_definitions(Unvoxeller_editor PRIVATE 
        UNVOXELLER_LIB
        )

    set_target_properties(Unvoxeller_editor PROPERTIES
        OUTPUT_NAME "Unvoxeller"
    )

target_include_directories(Unvoxeller_editor PRIVATE
    ${ROOT_DIR}/third/editor
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ROOT_DIR}/third/editor/imgui
)

target_link_libraries(Unvoxeller_editor PRIVATE
    imgui
    Unvoxeller
    glfw
    spdlog
)


if(WIN32)
    target_link_libraries(Unvoxeller_editor PRIVATE opengl32)

     target_compile_definitions(Unvoxeller_editor
        PRIVATE 
        GLFW_EXPOSE_NATIVE_WIN32
        )
    
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

    target_link_libraries(Unvoxeller_editor PRIVATE
        OpenGL::GL
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )

endif()
