cmake_minimum_required(VERSION 3.20)
project(nexus VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NEXUS_BUILD_TESTS "Build Nexus tests" ON)

add_executable(nexus
    compiler/main.cpp
    compiler/lexer/lexer.cpp
    compiler/parser/parser.cpp
    compiler/semantic/semantic.cpp
    compiler/codegen/llvm_ir.cpp
    compiler/driver/driver.cpp
)

target_include_directories(nexus PRIVATE compiler)
target_compile_features(nexus PRIVATE cxx_std_20)

if (MSVC)
  target_compile_options(nexus PRIVATE /W4 /permissive-)
else()
  target_compile_options(nexus PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

include(GNUInstallDirs)
install(TARGETS nexus RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NEXUS_BUILD_TESTS)
  enable_testing()
  add_executable(nexus_tests
      tests/test_main.cpp
      compiler/lexer/lexer.cpp
      compiler/parser/parser.cpp
      compiler/semantic/semantic.cpp
      compiler/codegen/llvm_ir.cpp
      compiler/driver/driver.cpp
  )
  target_include_directories(nexus_tests PRIVATE compiler)
  target_compile_features(nexus_tests PRIVATE cxx_std_20)
  if (MSVC)
    target_compile_options(nexus_tests PRIVATE /W4 /permissive-)
  else()
    target_compile_options(nexus_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
  add_test(NAME nexus_tests COMMAND nexus_tests)
endif()
