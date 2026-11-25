# SystemCore

A collection of reusable C++ components, concurrency utilities, design patterns, and system-level tools designed to simplify software development and build robust, scalable, and portable applications.

Public headers are under [`include/systemcore/`](./include/systemcore/). Include
the complete API with `systemcore/systemcore.hpp`, or include only the component
headers an application uses.

## Logging classes

The logging API is declared in [`include/systemcore/logging/`](./include/systemcore/logging/):

- **`systemcore::logging::Logger`** filters messages by a configurable minimum severity (default `kInfo`), creates a `LogRecord` with the current time and thread ID, and sends accepted records to each configured sink. Severity-specific helpers are available, and sink exceptions propagate to the caller.
- **`systemcore::logging::ConsoleSink`** formats records and writes warning-level and more severe messages to standard error; other messages go to standard output.
- **`systemcore::logging::FileSink`** appends formatted records to a file. Its `FlushPolicy` controls whether output is flushed manually, for warning-level and more severe records, or after every record.
- **`systemcore::logging::TextFormatter`** produces readable log lines. **`systemcore::logging::JsonFormatter`** produces JSON records with escaped message and thread-ID fields. Both format timestamps in UTC using millisecond precision.

Construct a logger with one or more sinks and a formatter, for example:

```cpp
#include "systemcore/logging/logging.hpp"

#include <memory>

using namespace systemcore::logging;

auto formatter = std::make_shared<TextFormatter>();
auto sink = std::make_shared<ConsoleSink>(formatter);
Logger logger({sink});
logger.info("Application started");
```

Logger tests are in [`tests/logging/`](./tests/logging/). Run them with:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'LoggerTest|ConsoleSinkTest|FileSinkTest|FormatterTest' --output-on-failure
```

## Scope timer

[`systemcore::benchmark::performance::ScopeTimer`](./include/systemcore/benchmark/performance/scope_timer.hpp) measures the elapsed time between construction and destruction, then prints the duration in milliseconds to standard output. Keep the characters referenced by its `std::string_view` name alive for the timer's entire lifetime. Its unit test is in [`tests/benchmark/performance/`](./tests/benchmark/performance/).

## File readers

The `systemcore::io::files` file reader API is declared in [`include/systemcore/io/files/`](./include/systemcore/io/files/):

- **`systemcore::io::files::FileReader`** opens an input file and records its path and size when constructed. The recorded size is a snapshot; it is not updated if the file changes.
- **`systemcore::io::files::BinaryFileReader`** reads binary data into a `std::vector<std::byte>`. `readAll()` reads from the beginning, while `readChunk(max_bytes)` reads up to the requested count from the current position. Both methods return `false` on failure and leave the output buffer empty.

Reads use the size captured at construction. Appended data is ignored, and truncating the file may cause a read to fail. `readChunk()` returns successfully with an empty buffer when no bytes remain.

## Concurrency

The concurrency API includes thread-safe queues and a circular buffer:

- [`systemcore::concurrency::Queue<T>`](./include/systemcore/concurrency/queue.hpp) is unbounded. `push()` accepts copies and moves; `tryPop()` retrieves immediately, and `waitPop(stop)` waits for an item or cancellation.
- [`systemcore::concurrency::BoundedQueue<T>`](./include/systemcore/concurrency/bounded_queue.hpp) has a fixed capacity (default `100`). `tryPush()` returns `false` when full; `waitPush(item, stop)` waits for capacity and returns `false` if stopped before space becomes available. `tryPop()` and `waitPop(stop)` provide immediate and cancellable retrieval.
- [`systemcore::concurrency::CircularBuffer<T>`](./include/systemcore/concurrency/circular_buffer.hpp) has a fixed capacity (default `100`) and overwrites the oldest element when a new element arrives while full. `pushOverwrite()` accepts copies and moves; `tryPop()` returns the oldest element or `std::nullopt` when empty. `reset()` discards all stored elements.

The `size()`/`count()`, `isEmpty()`, and `isFull()` methods return thread-safe snapshots, not reservations. Another thread can change a container immediately afterward; use its push/pop operation when the operation itself must be atomic. Neither queue currently has a close or shutdown operation.

Concurrency tests are in [`tests/concurrency/`](./tests/concurrency/). Run an individual suite with:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'QueueTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'BoundedQueueTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'CircularBufferTest' --output-on-failure
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

The tests are in [`tests/benchmark/performance/`](./tests/benchmark/performance/), [`tests/logging/`](./tests/logging/), [`tests/concurrency/`](./tests/concurrency/), and [`tests/io/files/`](./tests/io/files/). To run only one suite, filter by suite name:

```sh
/opt/homebrew/bin/ctest --test-dir build -R 'LoggerTest|ConsoleSinkTest|FileSinkTest|FormatterTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'ScopeTimerTest' --output-on-failure
/opt/homebrew/bin/ctest --test-dir build -R 'BinaryFileReaderTest' --output-on-failure
```
