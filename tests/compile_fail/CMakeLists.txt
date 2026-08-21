function(add_hfsm_compile_fail_test target)
  set(options)
  set(one_value_args EXPECTED_ERROR)
  set(multi_value_args SOURCES INCLUDE_DIRS)

  cmake_parse_arguments(
    ARG
    "${options}"
    "${one_value_args}"
    "${multi_value_args}"
    ${ARGN}
  )

  add_executable(${target} EXCLUDE_FROM_ALL)
  target_sources(${target} PRIVATE ${ARG_SOURCES})
  target_link_libraries(${target} PRIVATE hfsm::hfsm)

  if(ARG_INCLUDE_DIRS)
    target_include_directories(${target} PRIVATE ${ARG_INCLUDE_DIRS})
  endif()

  add_test(
    NAME ${target}
    COMMAND
      ${CMAKE_COMMAND}
        -DBUILD_DIR=${CMAKE_BINARY_DIR}
        -DTARGET=${target}
        -DEXPECTED_ERROR=${ARG_EXPECTED_ERROR}
        -P ${CMAKE_SOURCE_DIR}/cmake/check_compile_fail.cmake
  )
endfunction()

add_hfsm_compile_fail_test(
  invalid_state_when_call_get_state 
  SOURCES 
    invalid_state_when_call_get_state.cpp
  EXPECTED_ERROR
    "template parameter T must be a valid state type"
)

add_hfsm_compile_fail_test(
  not_derived_from_state_machine_def
  SOURCES 
    not_derived_from_state_machine_def.cpp
  EXPECTED_ERROR
    "template parameter must be derived from state_machine_def"
)

add_hfsm_compile_fail_test(
  lack_of_initial_state
  SOURCES 
    lack_of_initial_state.cpp
  EXPECTED_ERROR
    "initial_state"
)

add_hfsm_compile_fail_test(
  lack_of_transition_table
  SOURCES 
    lack_of_transition_table.cpp
  EXPECTED_ERROR
    "transition_table"
)