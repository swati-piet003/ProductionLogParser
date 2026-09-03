# Multithreaded Log Processing Engine

A portable C++17 command-line engine that streams large log files through a bounded producer/consumer pipeline. It counts log levels, filters records, and can write matches in their original input order.

## Features

- Configurable worker pool and bounded queues for predictable memory use
- Level and case-sensitive substring filters
- Thread-safe statistics for TRACE, DEBUG, INFO, WARN, ERROR, FATAL, and UNKNOWN
- Ordered filtered output despite parallel processing
- CMake build, dependency-free tests, and scripts for Windows and Linux
- GitHub Actions CI matrix for Windows and Linux
- Handles CRLF/LF files and timestamps using either a space or ISO-8601 `T`

Expected lines resemble:

```text
2026-01-15 10:23:48 [ERROR] Database connection timed out
WARN Cache is nearly full
```

Unrecognized lines are counted as `UNKNOWN`.

## Prerequisites

- CMake 3.16+
- A C++17 compiler: Visual Studio 2019+, GCC 8+, or Clang 7+

## Build and test

Windows PowerShell (run from the project directory):

```powershell
.\scripts\build.ps1
.\build\Release\log_processor.exe .\samples\application.log --level WARN --output errors.log
```

With a single-config generator such as Ninja, the executable may instead be at `build\log_processor.exe`.

Linux/macOS:

```bash
chmod +x scripts/build.sh
./scripts/build.sh
./build/log_processor samples/application.log --level WARN --output errors.log
```

For the included `samples/application.log`, the generated `errors.log` contains:

```text
2026-01-15 10:23:47 [WARN] Cache utilization at 85 percent
2026-01-15 10:23:48 [ERROR] Database connection timed out
2026-01-15T10:23:50Z [FATAL] Service cannot continue
```

The `--level WARN` filter includes `WARN` and every higher-severity level (`ERROR` and `FATAL`). The matching records remain in their original input order.

Manual CMake commands work on either platform:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## CLI

```text
log_processor <input-file> [options]
  -t, --threads N       Worker count (default: logical CPU count)
  -q, --queue-size N    Bounded queue size (default: 4096)
  -l, --level LEVEL     Minimum level: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
  -c, --contains TEXT   Only match lines containing TEXT (case-sensitive)
  -o, --output FILE     Write matching lines while preserving input order
```

Example:

```bash
log_processor server.log --threads 8 --level ERROR --contains database --output database-errors.log
```

Assume `server.log` contains:

```text
2026-01-15 11:00:00 [INFO] Server started
2026-01-15 11:00:01 [ERROR] database connection failed
2026-01-15 11:00:02 [ERROR] Request validation failed
2026-01-15 11:00:03 [FATAL] database is unavailable
```

The command prints statistics similar to:

```text
Processed 4 lines; matched 2
  TRACE: 0
  DEBUG: 0
   INFO: 1
   WARN: 0
  ERROR: 2
  FATAL: 1
UNKNOWN: 0
Elapsed: 0.001s
```

The elapsed time varies by system. The generated `database-errors.log` contains:

```text
2026-01-15 11:00:01 [ERROR] database connection failed
2026-01-15 11:00:03 [FATAL] database is unavailable
```

Both records satisfy the minimum `ERROR` level and contain the case-sensitive text `database`. The other `ERROR` record is excluded because it does not contain that text.

Statistics always cover every input line. `matched` reflects all active filters; output is only created when `--output` is provided.

## Architecture

The caller reads the input stream and applies backpressure through a bounded work queue. Worker threads parse and filter records while updating atomic counters. If output is enabled, results enter a second bounded queue and a dedicated writer reorders them by line number. No third-party runtime libraries are required.
