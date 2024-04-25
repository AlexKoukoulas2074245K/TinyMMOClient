cmake_minimum_required(VERSION 3.10)
project(TinyMMOClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build_utils")

# Enable folder use in CMake
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find SDL2
find_package(SDL2 REQUIRED COMPONENTS main)

# Find SDL2 Image
find_package(SDL2_image REQUIRED)

# Find OpenGL
if(APPLE)
  find_package(OpenGL REQUIRED)
endif(APPLE)

# "Find" glm
set(GLM_INCLUDE_DIRS "lib/glm")

# "Find" json
set(JSON_INCLUDE_DIRS "lib/json")

# Find freetype
find_package(Freetype REQUIRED)

# Platform specific directories + CMakeLists
if(IOS_PLATFORM)
  set(PLATFORM_DIRECTORY source_ios)
else()
  set(PLATFORM_DIRECTORY source_desktop)
endif()

# Platform specific utililties directories + CMakeLists
if(APPLE)
  set(PLATFORM_UTILITIES_DIRECTORY source_apple_utilities)
else()
  set(PLATFORM_UTILITIES_DIRECTORY source_windows_utilities)
endif()

# Add common includes
include_directories(source_common)
include_directories(${PLATFORM_DIRECTORY})
include_directories(${PLATFORM_UTILITIES_DIRECTORY})
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2main_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${JSON_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})

# Apple doesn't need an opengl loader thankfully
if(APPLE)
  include_directories(${OPENGL_INCLUDE_DIR})
endif(APPLE)

# Windows gtest necessity
if(WIN32)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

# Common directories + CMakeLists
add_subdirectory(${PLATFORM_DIRECTORY})
add_subdirectory(${PLATFORM_UTILITIES_DIRECTORY})
add_subdirectory(source_common)
add_subdirectory(lib/googletest)
add_subdirectory(source_test)

# Enable highest warning levels + treated as errors
if(WIN32)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  target_compile_options(${PROJECT_NAME}_platform PRIVATE /W4)
  target_compile_options(${PROJECT_NAME}_platform_utilities PRIVATE /W4)
  target_compile_options(${PROJECT_NAME}_test PRIVATE /W4)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(${PROJECT_NAME}_platform PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(${PROJECT_NAME}_platform_utilities PRIVATE -Wall -Wextra -pedantic -Werror)
  target_compile_options(${PROJECT_NAME}_test PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# Put these targets in the 'HiddenTargets' folder in the IDE. 
set_target_properties(gmock gmock_main gtest gtest_main TinyMMOClient_lib PROPERTIES FOLDER HiddenTargets)

# Put these targets in the 'ProjectTargets' folder in the IDE.
set_target_properties(TinyMMOClient TinyMMOClient_platform TinyMMOClient_test TinyMMOClient_platform_utilities PROPERTIES FOLDER ProjectTargets)


