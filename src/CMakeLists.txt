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

set(wallpaperd_CFLAGS "${PC_IMLIB2_CFLAGS} -I${X11_INCLUDE_DIR}")
set(wallpaperd_LIBRARIES "${PC_IMLIB2_LDFLAGS} ${PC_IMLIB2_LDFLAGS_OTHER} ${X11_LIBRARIES}")

if (PC_XRANDR_FOUND)
  set(wallpaperd_CFLAGS "${wallpaperd_CFLAGS} ${PC_XRANDR_CFLAGS}")
  set(wallpaperd_LIBRARIES ${wallpaperd_LIBRARIES} ${PC_XRANDR_LDFLAGS} ${PC_XRANDR_LDFLAGS_OTHER})
endif (PC_XRANDR_FOUND)

add_executable(wallpaperd ${wallpaperd_SOURCES})
set_target_properties(wallpaperd PROPERTIES COMPILE_FLAGS "${wallpaperd_CFLAGS}")
target_link_libraries(wallpaperd ${wallpaperd_LIBRARIES})
