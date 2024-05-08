# Function to preserve source tree hierarchy of project
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

set(BINARY ${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE SOURCES *.h *.cpp *c)

set(SOURCES ${SOURCES})

add_executable(${BINARY} ${SOURCES})
add_library(${BINARY}_lib STATIC ${SOURCES})

if(NOT APPLE)
  set(OPENGL_LIBRARIES opengl32.lib)
endif()

if(NOT APPLE)
  set(WININET_LIBRARIES Wininet.lib)
  set(WININET_DLLS Wininet.dll)
endif()

target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_platform ${PROJECT_NAME}_platform_utilities ${PROJECT_NAME}_net_common ${SDL2_LIBS} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${WININET_LIBRARIES} ${FREETYPE_LIBRARIES})

assign_source_group(${SOURCES})

# Copy DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS} ${SDL2IMAGE_DLLS} ${FREETYPE_DLLS})
        message("Copying ${DLL} to main output folder")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach()
endif()
