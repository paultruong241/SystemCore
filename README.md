# SystemCore

A collection of reusable C++ components, concurrency utilities, design patterns, and system-level tools designed to simplify software development and build robust, scalable, and portable applications.

## Logging classes

The logging API is declared in [`include/logging/`](./include/logging/):

- **`LogLevel`** defines the ordered severity levels `kTrace`, `kDebug`, `kInfo`, `kWarning`, `kError`, and `kFatal`. `logLevelToString` converts a level to its label.
- **`Logger`** is the abstract base class. It stores a minimum severity threshold, synchronizes output, and formats log entries with a timestamp, severity, and thread ID.
- **`ConsoleLogger`** writes accepted messages to standard output. Warning and more severe messages are flushed immediately.
- **`FileLogger`** writes accepted messages to a timestamp-named file in the current working directory. Warning and more severe messages are flushed immediately; open and write failures are reported to standard error.

Both concrete loggers default to the `kInfo` threshold and omit messages below their configured threshold.

## File readers

The file reader API is declared in [`include/io/files/`](./include/io/files/):

- **`FileReader`** opens an input file and records its path and size when constructed. The recorded size is a snapshot; it is not updated if the file changes.
- **`BinaryFileReader`** reads binary data into a `std::vector<std::byte>`. `readAll()` reads from the beginning, while `readChunk(max_bytes)` reads up to the requested count from the current position. Both methods return `false` on failure and leave the output buffer empty.

Reads use the size captured at construction. Appended data is ignored, and truncating the file may cause a read to fail. `readChunk()` returns successfully with an empty buffer when no bytes remain.

## Thread-safe queue

[`include/containers/thread_safe_queue.hpp`](./include/containers/thread_safe_queue.hpp) provides the header-only `ThreadSafeQueue<T>`, a mutex-protected FIFO queue with copy and move `push` overloads. Its retrieval methods move values out of the queue:

- `wait_pop(std::stop_token)` blocks until a value is available or stop is
  requested. It returns an empty optional if stopped first.
- `try_pop()` returns immediately with an `std::optional<T>`, which is empty if the queue has no value.
- `wait_pop_for(timeout)` waits for the specified `std::chrono::milliseconds` duration and returns an empty optional if it times out.

Use the stop-aware overload to cancel a waiting consumer. The queue has no
close or shutdown operation.

## Build and test prerequisites

The build requires CMake and GoogleTest. The project uses GoogleTest for unit tests and CTest to discover and run them. The VS Code tasks in [`.vscode/tasks.json`](./.vscode/tasks.json) currently use Homebrew's Apple Silicon paths (`/opt/homebrew/bin`).

## Run the VS Code tasks

Open the Command Palette with **Cmd+Shift+P** and choose **Tasks: Run Task** to select a task by name:

- **CMake: Configure (tests enabled)** configures the project into `build/` and enables tests. It generates build files; it does not compile or run tests.
- **CMake: Build (including tests)** configures first, then builds the library and GoogleTest executable. This is the default build task and can also be run with **Cmd+Shift+B**.
- **CTest: Run tests** builds first, then runs the discovered tests with failure output enabled. Run it from **Tasks: Run Test Task** or **Tasks: Run Task**.

The tasks use `/opt/homebrew/bin/cmake` and `/opt/homebrew/bin/ctest`. If those executables are elsewhere on your system, update their paths in `tasks.json`.

## Run from a terminal

From the repository root, the following commands configure, build, and run all tests:

```sh
/opt/homebrew/bin/cmake -S . -B build -DBUILD_TESTING=ON
/opt/homebrew/bin/cmake --build build
/opt/homebrew/bin/ctest --test-dir build --output-on-failure
```

The tests are in [`tests/logging/`](./tests/logging/), [`tests/containers/`](./tests/containers/), and [`tests/io/files/`](./tests/io/files/). To run only one suite, filter by suite name:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'ConsoleLoggerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'FileLoggerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'BinaryFileReaderTest' --output-on-failure
```
