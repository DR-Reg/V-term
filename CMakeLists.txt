cmake_minimum_required(VERSION 3.24)
include(FetchContent)

set(PROJECT_NAME vterm)
project(${PROJECT_NAME} C)

set(SOURCE_FILES main.c vterm.c)
set(INCLUDE_DIRS fonts/headers)


# Fetching raylib from github
set(RAYLIB_VERSION 5.0)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS ${RAYLIB_VERSION} EXACT
)
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(raylib)

# Add executables
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_executable(make_font_headers fonts/make_font_headers.c)

# Link to libraries
target_link_libraries(${PROJECT_NAME} raylib)
target_link_libraries(make_font_headers raylib)

# Include fonts
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIRS})
    
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")

    target_link_libraries(make_font_headers "-framework IOKit")
    target_link_libraries(make_font_headers "-framework Cocoa")
    target_link_libraries(make_font_headers "-framework OpenGL")
endif()
