include_directories(
  "../include"
)

enable_testing()

add_executable( tests
  test.cpp
  unique-queue-tests.cpp
)
target_link_libraries(
  tests
  GTest::gtest_main
)
include(GoogleTest)
gtest_discover_tests(tests)