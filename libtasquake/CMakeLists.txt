list(APPEND LIBTASQUAKE_SOURCES
  "src/game_funcs.cpp"
  "src/script_parse.cpp"
  "src/script_playback.cpp"
  "src/ipc.cpp"
  "src/utils.cpp"
)

add_library(libtasquake ${LIBTASQUAKE_SOURCES})
target_include_directories(libtasquake PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(libtasquake INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(test)
