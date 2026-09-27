if(NOT DEFINED NEXUS_BIN)
  message(FATAL_ERROR "NEXUS_BIN is required")
endif()
set(SMOKE_DIR "${CMAKE_BINARY_DIR}/project-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")

execute_process(
  COMMAND "${NEXUS_BIN}" new "${SMOKE_DIR}" --template cli
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus new failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" info
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "version: 1[.]1[.]0")
  message(FATAL_ERROR "nexus info failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" assets
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "beep\\.wav" OR NOT out MATCHES "cube\\.obj")
  message(FATAL_ERROR "nexus assets failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "nexus check failed: ${out}\n${err}")
endif()

execute_process(
  COMMAND "${NEXUS_BIN}" build --release
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT EXISTS "${SMOKE_DIR}/build/release/project-smoke")
  message(FATAL_ERROR "nexus build failed: ${out}\n${err}")
endif()


execute_process(
  COMMAND "${NEXUS_BIN}" fmt
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "nexus fmt failed: ${out}\n${err}")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" lint
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "lint: OK")
  message(FATAL_ERROR "nexus lint failed: ${out}\n${err}")
endif()
execute_process(
  COMMAND "${NEXUS_BIN}" editor
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT EXISTS "${SMOKE_DIR}/.vscode/settings.json")
  message(FATAL_ERROR "nexus editor integration failed: ${out}\n${err}")
endif()
file(WRITE "${SMOKE_DIR}/src/main.nx" "fn main(){ const base: i64 = 2; let mut total: i64 = 0; for i in 1..6 { total += i * base } print(total) }\n")
execute_process(
  COMMAND "${NEXUS_BIN}" check
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
  message(FATAL_ERROR "language v1 syntax smoke failed: ${out}\n${err}")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "NEXUS project workflow smoke test: PASS")
