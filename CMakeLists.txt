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

  include(GoogleTest)

  function(add_hfsm_test target)
    add_executable(${target})
    target_sources(${target} PRIVATE ${ARGN})
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror -O2 -g)
    target_link_libraries(${target} PRIVATE hfsm GTest::gtest_main)
    gtest_discover_tests(${target})
  endfunction()

  add_hfsm_test(test_hfsm_mpl tests/test_hfsm_mpl.cpp)
  add_hfsm_test(test_hierarchical_state_machine tests/test_hierarchical_state_machine.cpp)
  add_hfsm_test(test_simple_single_level_state_machine tests/test_simple_single_level_state_machine.cpp)
  add_hfsm_test(test_three_level_state_machine tests/test_three_level_state_machine.cpp)
  
endif()