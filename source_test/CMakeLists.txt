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

set(BINARY ${CMAKE_PROJECT_NAME}_test)

file(GLOB_RECURSE TEST_SOURCES *.h *.cpp *c)

set(SOURCES ${TEST_SOURCES})

add_executable(${BINARY} ${TEST_SOURCES})

add_test(NAME ${BINARY} COMMAND ${BINARY})

if(NOT APPLE)
  set(OPENGL_LIBRARIES opengl32.lib)
endif()

target_link_libraries(${BINARY} PUBLIC ${CMAKE_PROJECT_NAME}_lib ${PROJECT_NAME}_platform ${PROJECT_NAME}_net_common gtest ${SDL2_LIBS} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${FREETYPE_LIBRARIES})

assign_source_group(${TEST_SOURCES})

# Copy DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${SDL2_DLLS} ${SDL2IMAGE_DLLS} ${FREETYPE_DLLS})
        message("Copying ${DLL} to test output folder")
        add_custom_command(TARGET  ${PROJECT_NAME}_test POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}_test>)
    endforeach()
endif()
