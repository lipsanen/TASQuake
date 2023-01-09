list(APPEND LIBTASQUAKE_TEST_SOURCES
  "bench.cpp"
  "bench_frameblock.cpp"
  "catch_amalgamated.cpp"
  "cliff_tests.cpp"
  "framedata_tests.cpp"
  "rollingstone_test.cpp"
  "optimizer_test.cpp"
  "script_tests.cpp"
  "test.cpp"
)

add_executable(libtasquake_test ${LIBTASQUAKE_TEST_SOURCES})
target_link_libraries(libtasquake_test PRIVATE libtasquake)
add_test(NAME LIBTASQUAKE_TEST COMMAND libtasquake_test --skip-benchmarks)

add_subdirectory(server_test)
