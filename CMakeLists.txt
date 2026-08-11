cmake_minimum_required(VERSION 3.20)

project(hfsm VERSION 0.01 LANGUAGES CXX)

add_library(hfsm INTERFACE)

target_include_directories(hfsm INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_compile_features(hfsm INTERFACE cxx_std_14)

include(CTest)

if (BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.17.0
      GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(test_hfsm)
  target_sources(test_hfsm PRIVATE tests/test_hfsm.cpp)
  target_compile_options(test_hfsm PRIVATE -Wall -Wextra -Wpedantic -Werror -O2 -g)
  target_link_libraries(test_hfsm PRIVATE hfsm GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(test_hfsm)
endif()