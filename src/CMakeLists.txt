cmake_minimum_required(VERSION 2.8.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories("${PROJECT_SOURCE_DIR}/src")

set(wallpaperd_SOURCES
  background.c
  background_xml.c
  cache.c
  compat.c
  cfg.c
  main.c
  render.c
  wallpaper.c
  wallpaper_match.c
  util.c
  x11.c)

set(wallpaperd_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/src ${X11_INCLUDE_DIR})
set(wallpaperd_LIBRARIES ${X11_LIBRARIES})

if (X11_Xrandr_FOUND)
  set(wallpaperd_INCLUDE_DIRS ${wallpaperd_INCLUDE_DIRS} ${X11_Xrandr_INCLUDE_PATH})
  set(wallpaperd_LIBRARIES ${wallpaperd_LIBRARIES} ${X11_Xrandr_LIB})
endif (X11_Xrandr_FOUND)

set(wallpaperd_INCLUDE_DIRS ${wallpaperd_INCLUDE_DIRS} ${Imlib2_INCLUDE_DIR})
set(wallpaperd_LIBRARIES ${wallpaperd_LIBRARIES} ${Imlib2_LIBRARIES})

add_executable(wallpaperd ${wallpaperd_SOURCES})
target_include_directories(wallpaperd PUBLIC ${wallpaperd_INCLUDE_DIRS})
target_link_libraries(wallpaperd ${wallpaperd_LIBRARIES})

install(TARGETS wallpaperd DESTINATION bin)
