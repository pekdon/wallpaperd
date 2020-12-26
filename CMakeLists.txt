cmake_minimum_required(VERSION 2.8.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules/")

project(wallpaperd)
set(wallpaperd_VERSION_MAJOR 0)
set(wallpaperd_VERSION_MINOR 2)
set(wallpaperd_VERSION_MICRO 3)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=c99)

find_package(X11 REQUIRED)
find_package(Imlib2 REQUIRED)

check_function_exists(arc4random HAVE_ARC4RANDOM)
check_function_exists(daemon HAVE_DAEMON)
check_function_exists(strlcat HAVE_STRLCAT)

configure_file("${PROJECT_SOURCE_DIR}/config.h.in"
               "${PROJECT_BINARY_DIR}/config.h")
include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH True)

add_subdirectory(src)
