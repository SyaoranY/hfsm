cmake_minimum_required(VERSION 3.20)

project(hfsm VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(hfsm INTERFACE)
add_library(hfsm::hfsm ALIAS hfsm)

target_include_directories(
  hfsm
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(hfsm INTERFACE cxx_std_14)

include(CTest)

if (BUILD_TESTING)
  include(FetchContent)
  # GoogleTest is only a test dependency of hfsm.
  # Do not install gtest/gmock together with hfsm.
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

  # hfsm currently uses GoogleTest only, not GoogleMock.
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  
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
    target_link_libraries(${target} PRIVATE hfsm::hfsm GTest::gtest_main)
    gtest_discover_tests(${target})
  endfunction()

  add_hfsm_test(test_hfsm_mpl tests/test_hfsm_mpl.cpp)
  add_hfsm_test(test_hierarchical_state_machine tests/test_hierarchical_state_machine.cpp)
  add_hfsm_test(test_simple_single_level_state_machine tests/test_simple_single_level_state_machine.cpp)
  add_hfsm_test(test_three_level_state_machine tests/test_three_level_state_machine.cpp)
  
endif()


# ------------------------------------------------------------
# Install library
# ------------------------------------------------------------

install(
  TARGETS hfsm
  EXPORT hfsmTargets
)

install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ------------------------------------------------------------
# Export CMake target
# ------------------------------------------------------------

install(
  EXPORT hfsmTargets
  FILE hfsmTargets.cmake
  NAMESPACE hfsm::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hfsm
)

# ------------------------------------------------------------
# Package config
# ------------------------------------------------------------

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hfsmConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/hfsmConfig.cmake
  INSTALL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/hfsm
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/hfsmConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hfsmConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hfsmConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/hfsm
)