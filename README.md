# SystemCore

A collection of reusable C++ components, concurrency utilities, design patterns, and system-level tools designed to simplify software development and build robust, scalable, and portable applications.

## Logging classes

The logging API is declared in [`include/logging/`](./include/logging/):

- **`logging::LogLevel`** defines the ordered severity levels `kTrace`, `kDebug`, `kInfo`, `kWarning`, `kError`, and `kFatal`. `logging::logLevelToString` converts a level to its label.
- **`logging::Logger`** is the abstract base class. It stores a minimum severity threshold, synchronizes output, and formats log entries with a timestamp, severity, and thread ID.
- **`logging::ConsoleLogger`** is a process-wide singleton that writes accepted messages to standard output. `logging::ConsoleLogger::instance(level)` uses `level` only on its first call; later calls return the same logger and ignore the argument. The default threshold is `kInfo`.
- **`logging::FileLogger`** writes accepted messages to a caller-supplied file or, by default, a timestamp-named file in the current working directory. Warning and more severe messages are flushed immediately; open and write failures are reported to standard error. Each instance has its own file and threshold.

Both concrete loggers omit messages below their threshold, which is fixed when each logger is constructed.

## Scope timer

[`benchmark::performance::ScopeTimer`](./include/benchmark/performance/scope_timer.hpp) measures the elapsed time between construction and destruction, then prints the duration in milliseconds to standard output. Keep the characters referenced by its `std::string_view` name alive for the timer's entire lifetime. Its unit test is in [`tests/benchmark/performance/`](./tests/benchmark/performance/).

## File readers

The `io::files` file reader API is declared in [`include/io/files/`](./include/io/files/):

- **`io::files::FileReader`** opens an input file and records its path and size when constructed. The recorded size is a snapshot; it is not updated if the file changes.
- **`io::files::BinaryFileReader`** reads binary data into a `std::vector<std::byte>`. `readAll()` reads from the beginning, while `readChunk(max_bytes)` reads up to the requested count from the current position. Both methods return `false` on failure and leave the output buffer empty.

Reads use the size captured at construction. Appended data is ignored, and truncating the file may cause a read to fail. `readChunk()` returns successfully with an empty buffer when no bytes remain.

## Concurrent containers

The containers API includes two mutex-protected FIFO queues and a circular buffer:

- [`containers::ConcurrentQueue<T>`](./include/containers/concurrent_queue.hpp) is unbounded. `push()` accepts copies and moves; `tryPop()` retrieves immediately, and `waitPop(stop)` waits for an item or cancellation.
- [`containers::ConcurrentBoundedQueue<T>`](./include/containers/concurrent_bounded_queue.hpp) has a fixed capacity (default `100`). `tryPush()` returns `false` when full; `waitPush(item, stop)` waits for capacity and returns `false` if stopped before space becomes available. `tryPop()` and `waitPop(stop)` provide immediate and cancellable retrieval.
- [`containers::ConcurrentCircularBuffer<T>`](./include/containers/concurrent_circular_buffer.hpp) has a fixed capacity (default `100`) and overwrites the oldest element when a new element arrives while full. `pushOverwrite()` accepts copies and moves; `tryPop()` returns the oldest element or `std::nullopt` when empty. `reset()` discards all stored elements.

The `size()`/`count()`, `isEmpty()`, and `isFull()` methods return thread-safe snapshots, not reservations. Another thread can change a container immediately afterward; use its push/pop operation when the operation itself must be atomic. Neither queue currently has a close or shutdown operation.

Container tests are in [`tests/containers/`](./tests/containers/). Run an individual suite with:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'ConcurrentQueueTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'ConcurrentBoundedQueueTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'ConcurrentCircularBufferTest' --output-on-failure
```

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

The tests are in [`tests/benchmark/performance/`](./tests/benchmark/performance/), [`tests/logging/`](./tests/logging/), [`tests/containers/`](./tests/containers/), and [`tests/io/files/`](./tests/io/files/). To run only one suite, filter by suite name:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'ConsoleLoggerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'FileLoggerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'ScopeTimerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'BinaryFileReaderTest' --output-on-failure
```
