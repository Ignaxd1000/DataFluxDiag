cmake_minimum_required(VERSION 3.16)
project(DataFluxDiag VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(WIN32)
  message(FATAL_ERROR "Windows support is planned for a future release and is intentionally not implemented yet.")
endif()

option(DATAFLUXDIAG_BUILD_TESTS "Build DataFluxDiag tests" ON)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(DATAFLUXDIAG_COMMON_SOURCES
  src/cli.c
  src/buffer_diag.c
  src/scanner.c
  src/transfer.c
  src/utils.c
  src/platform/posix/io_posix.c
  src/platform/posix/platform_posix.c
  src/platform/posix/socket_posix.c
)

add_library(datafluxdiag_core STATIC ${DATAFLUXDIAG_COMMON_SOURCES})
target_include_directories(datafluxdiag_core PUBLIC include)
target_compile_definitions(datafluxdiag_core PRIVATE _POSIX_C_SOURCE=200809L)

add_executable(datafluxdiag
  src/main.c
)
target_link_libraries(datafluxdiag PRIVATE datafluxdiag_core)

if(DATAFLUXDIAG_BUILD_TESTS)
  enable_testing()
  add_executable(datafluxdiag_tests tests/test_main.c)
  target_link_libraries(datafluxdiag_tests PRIVATE datafluxdiag_core)
  add_test(NAME datafluxdiag_tests COMMAND datafluxdiag_tests)
endif()
