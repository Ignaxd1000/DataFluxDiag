cmake_minimum_required(VERSION 3.16)
project(StreamDiag VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(WIN32)
  message(FATAL_ERROR "Windows support is planned for a future release and is intentionally not implemented yet.")
endif()

option(STREAMDIAG_BUILD_TESTS "Build StreamDiag tests" ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(STREAMDIAG_COMMON_SOURCES
  src/main.c
  src/cli.c
  src/buffer_diag.c
  src/scanner.c
  src/transfer.c
  src/utils.c
  src/platform/posix/io_posix.c
  src/platform/posix/platform_posix.c
  src/platform/posix/socket_posix.c
)

add_library(streamdiag_core STATIC ${STREAMDIAG_COMMON_SOURCES})
target_include_directories(streamdiag_core PUBLIC include)
target_compile_definitions(streamdiag_core PRIVATE _POSIX_C_SOURCE=200809L)

add_executable(streamdiag src/entrypoint.c)
target_link_libraries(streamdiag PRIVATE streamdiag_core)

if(STREAMDIAG_BUILD_TESTS)
  enable_testing()
  add_executable(streamdiag_tests tests/test_main.c)
  target_link_libraries(streamdiag_tests PRIVATE streamdiag_core)
  add_test(NAME streamdiag_tests COMMAND streamdiag_tests)
endif()
