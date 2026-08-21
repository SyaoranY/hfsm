execute_process(
  COMMAND ${CMAKE_COMMAND} --build "${BUILD_DIR}" --target "${TARGET}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)

set(all_output "${output}\n${error}")

# 1. 必须编译失败
if(result EQUAL 0)
  message(FATAL_ERROR
    "Expected '${TARGET}' to fail compilation, but it compiled successfully."
  )
endif()

# 2. 必须包含预期错误
string(FIND
  "${all_output}"
  "${EXPECTED_ERROR}"
  error_position
)

if(error_position EQUAL -1)
  message(FATAL_ERROR
    "Compilation failed, but expected error was not found.\n"
    "Expected: ${EXPECTED_ERROR}\n\n"
    "Compiler output:\n${all_output}"
  )
endif()

message(STATUS "Compile-fail test passed: ${EXPECTED_ERROR}")