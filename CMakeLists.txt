cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(PROJECT_NAME vterm)
project(${PROJECT_NAME} C)

set(SOURCE_FILES main.c vterm.c)


# Fetching raylib from github
set(RAYLIB_VERSION 5.0)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS ${RAYLIB_VERSION} EXACT
)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Link to libraries
target_link_libraries(${PROJECT_NAME} raylib)
    
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
