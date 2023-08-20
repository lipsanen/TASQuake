list(APPEND LIBTASQUAKE_SOURCES
  "src/boost_ipc.cpp"
  "src/console_input.c"
  "src/game_funcs.cpp"
  "src/draw.cpp"
  "src/io.cpp"
  "src/optimizer.cpp"
  "src/prediction.cpp"
  "src/script_parse.cpp"
  "src/script_playback.cpp"
  "src/ipc.cpp"
  "src/utils.cpp"
  "src/vector.cpp"
)

add_library(libtasquake ${LIBTASQUAKE_SOURCES})
target_include_directories(libtasquake PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(libtasquake INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(test)
