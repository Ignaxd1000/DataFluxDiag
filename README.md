# StreamDiag

StreamDiag is an academic but practical CLI diagnostics project for Operating Systems concepts: **program-managed buffers**, **low-level file I/O**, and **TCP sockets**.

This repository currently provides a Linux/POSIX implementation with a modular architecture that keeps platform-specific code inside dedicated platform layers.

## Scope (current version)

Implemented features:
- Buffer Diagnostic (read/copy benchmarks with custom dynamic buffer sizes)
- TCP Socket Scanner (IPv4 TCP connect scanner with timeout)
- TCP File Transfer (custom simple protocol over TCP)

Out of scope by design:
- pipes
- FTP
- UDP
- IPv6
- GUI
- raw sockets
- packet capture/crafting
- encryption/authentication
- databases/daemons
- advanced Nmap-like features

## Platform support

- ✅ Linux/POSIX: implemented and buildable
- ⏳ Windows: **intentionally deferred for a future release**

When compiling for Windows, CMake fails explicitly with a clear message to avoid ambiguous partial support.

## Build requirements

- CMake >= 3.16
- C compiler with C11 support (GCC/Clang tested on Linux)

## Build

### Linux

```bash
cmake -S . -B build
cmake --build build
```

### Windows (current state)

```bash
cmake -S . -B build
cmake --build build --config Release
```

This currently fails intentionally with:

> Windows support is planned for a future release and is intentionally not implemented yet.

## CLI

### Interactive mode

Run without arguments:

```bash
./build/streamdiag
```

Menu:

```text
StreamDiag
1. Buffer Diagnostic
2. TCP Socket Scanner
3. TCP File Transfer
0. Exit
```

### Help

```bash
./build/streamdiag --help
```

### Commands

#### Buffer benchmark

```bash
./build/streamdiag buffer benchmark <file> [--copy-out <output_file>] [--buffers <list_or_range>]
```

Examples:

```bash
./build/streamdiag buffer benchmark data.bin
./build/streamdiag buffer benchmark data.bin --buffers 512,1024,4096,65536
./build/streamdiag buffer benchmark data.bin --buffers 1024-65536:1024
./build/streamdiag buffer benchmark input.bin --copy-out output.bin --buffers 4096,16384,65536
```

Output includes:
- file size
- selected buffer size
- operation count
- bytes processed
- elapsed time
- throughput (MiB/s)

Default buffer size (if not provided): **65536 bytes**.

#### TCP scanner

```bash
./build/streamdiag scan <host_or_ipv4> <start-end> [--timeout <ms>]
```

Example:

```bash
./build/streamdiag scan 127.0.0.1 20-100 --timeout 750
```

Each port reports:
- OPEN
- CLOSED
- TIMEOUT
- latency when available

> Authorization warning: only scan hosts and networks you are authorized to test.

#### TCP file transfer

Server:

```bash
./build/streamdiag server --port 5000
```

Client:

```bash
./build/streamdiag send 127.0.0.1:5000 archivo.bin
```

## Transfer protocol

Simple custom TCP protocol (not FTP):

1. Fixed binary header (network order):
   - `magic` (u32): `SDTF`
   - `version` (u16): `1`
   - `filename_length` (u16)
   - `file_size` (u64)
2. Filename bytes (`filename_length`)
3. File bytes (`file_size`)
4. Server sends 1-byte completion ACK (`1` success, `0` failure)

The server stores data as `received_<basename>` and validates `received_bytes == expected_file_size`.

## Architecture

```text
include/
  cli.h
  buffer_diag.h
  scanner.h
  transfer.h
  utils.h
  platform/
    io.h
    socket.h
    platform.h
src/
  entrypoint.c
  main.c
  cli.c
  buffer_diag.c
  scanner.c
  transfer.c
  utils.c
  platform/
    posix/
      io_posix.c
      socket_posix.c
      platform_posix.c
    windows/
      io_windows.c           (deferred placeholder)
      socket_windows.c       (deferred placeholder)
      platform_windows.c     (deferred placeholder)
tests/
  test_main.c
```

Design goals:
- small and maintainable code
- platform-specific code concentrated in platform modules
- no scattered OS checks in business logic
- clear error messages and return-value checking

## Tests

Offline tests (no external network dependency) cover:
- buffer size validation
- buffer list/range parsing
- host:port parsing
- port range parsing
- throughput math
- checksum helper stability

Run:

```bash
ctest --test-dir build --output-on-failure
```

## Limitations

- Current scanner is sequential for simplicity and maintainability.
- IPv4-only TCP scanner and transfer in this version.
- Windows backend intentionally deferred.

## Future improvements

- Implement Windows platform backends (I/O, timing, sockets)
- Optional bounded concurrency for scanner
- Optional transfer resume and richer status reporting
- Additional parser tests and protocol robustness checks
