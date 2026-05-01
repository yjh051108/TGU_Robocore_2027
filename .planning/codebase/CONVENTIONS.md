# Coding Conventions

**Analysis Date:** 2026-05-01

## Naming Patterns

**Files:**
- Source files: `lower_snake_case.cpp` -- e.g., `serial.cpp`, `foxglove_comm.cpp`, `auto_aim.cpp`
- Header files: `lower_snake_case.hpp` -- e.g., `serial.hpp`, `logger.hpp`, `crc.hpp`
- Test files: `test_xxx.cpp` -- e.g., `test_serial.cpp`, `test_logger.cpp`, `test_camera.cpp`
- Config files: `lower_snake_case.toml` -- e.g., `testconfig.toml`

**Functions:**
- Lower snake case: `spin_once()`, `get_crc16()`, `check_crc8()`, `is_open()`
- Boolean predicates prefixed with `is_`, `has_`, `check_`: `is_open()`, `check_crc16()`
- Some legacy camelCase exists (the project style guide at `项目结构与代码风格.md` notes `isOpen()` / `spinOnce()` mixed usage -- the codebase currently uses `is_open()` and `spin_once()`, consistent with the recommended `lower_snake_case`)

**Variables:**
- Local variables: `lower_snake_case` -- e.g., `int baudrate;`, `size_t rx_len;`, `std::string file_path;` (`serial.hpp`)
- Member variables: trailing underscore -- e.g., `level_`, `console_`, `file_`, `ofs_`, `mutex_` (`logger.hpp`), `is_open_`, `rx_buf_` (`serial.hpp`), `host`, `port`, `ready` (PImpl in `foxglove_comm.cpp` -- note: the PImpl struct uses non-underscore member names, inconsistent with the convention)
- Constants: `UPPER_SNAKE_CASE` or `constexpr` -- e.g., `CRC16_INIT = 0xffff`, `HEAD_SIZE = 2`, `static constexpr const char* MODULE = "SERIAL"` (`serial.hpp:143`, `crc.cpp:23`)

**Types:**
- Classes: `UpperCamelCase` -- e.g., `Serial`, `Logger`, `FoxGloveComm`, `RingBuffer`, `StructParser`
- Structs: `UpperCamelCase` -- e.g., `LoggerConfig`, `RecvPackage`, `SendPackage`
- Enums: `UpperCamelCase` -- e.g., `LogLevel` (scoped enum)

**Namespaces:**
- Lowercase: `namespace io { }`, `namespace tools { }`
- Proposed for future: `namespace app { namespace auto_aim { } }` (documented in `项目结构与代码风格.md`)

## Code Style

**Formatting:**
- Indentation: 4 spaces (consistent across most files; `crc.cpp` uses 2-space indentation inconsistently)
- No tabs
- Brace style: Attach (opening brace on same line as statement) -- e.g., `bool Serial::open(...) {`
- Column limit: not formally enforced (recommended 100 in style guide)
- Pointer alignment: Left (`const uint8_t* data`)

**Current inconsistencies:**
- `tools/crc.cpp` uses 2-space indent while the rest of the codebase uses 4-space indent
- `tools/logger.hpp` uses 4-space indent with a mix of indentation styles inside namespaces (4-space consistently inside `namespace tools {}` but some continuation lines vary)
- `io/serial/serial.hpp` namespace content is 4-space indented, consistent with the dominant style

**Linting:**
- No `.clang-format` file present (one is recommended in `项目结构与代码风格.md`)
- No automated linting pipeline detected

**Recommended `.clang-format`** (from `项目结构与代码风格.md`):
```yaml
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Attach
AllowShortFunctionsOnASingleLine: Empty
PointerAlignment: Left
SortIncludes: true
NamespaceIndentation: None
```

## Import Organization

**Preferred order** (from `项目结构与代码风格.md`):
1. Current module's header file (in `.cpp` files)
2. C/C++ standard library headers
3. Third-party library headers (Boost, OpenCV, etc.)
4. Project internal headers

**Observed patterns:**
- `serial.cpp` follows this order exactly: `#include "serial.hpp"` -> standard headers (none) -> `#include <boost/asio.hpp>` -> `#include "tools/logger.hpp"` (`serial.cpp:5-6`)
- `test_serial.cpp`: standard headers first -> project headers: `#include <iostream>` -> `#include "io/serial/serial.hpp"` (note: this does NOT follow the recommended order -- the test puts project headers second without the current module header)
- `foxglove_comm.cpp`: `#include "foxglove_comm.hpp"` -> standard headers -> `#include "foxglove/server.hpp"` -> `#include "tools/logger.hpp"` (follows recommended order)

**Path Aliases:**
- All includes use full relative paths from project root: `"io/serial/serial.hpp"`, `"tools/logger.hpp"`, `"tools/crc.hpp"`
- No CMake `target_include_directories()` alias usage -- paths resolve from `include_directories(${PROJECT_SOURCE_DIR})` set in the top-level `CMakeLists.txt`

## Error Handling

**Patterns:**
- Device initialization functions return `bool`: `open()` returns `true`/`false` (`serial.cpp:17`)
- On failure, `LOG_ERROR` is called before returning `false` (`serial.cpp:38-41`)
- On success, `LOG_INFO` is called before returning `true` (`serial.cpp:35-36`)
- Exceptions are caught at function boundaries in `io` layer (`serial.cpp:38`, `serial.cpp:61`)
- Empty `catch (...)` is used in `serial.cpp:61` and `serial.cpp:77` -- this is flagged as an anti-pattern in the style guide

**Example of recommended pattern** (`serial.cpp:17-42`):
```cpp
bool Serial::open(const std::string& device, int baudrate) {
    try {
        // ... setup ...
        is_open_ = true;
        LOG_INFO(MODULE, "{} open success", device);
        return true;
    } catch (std::exception& e) {
        LOG_ERROR(MODULE,"{} open failed: {}", device, e.what());
        is_open_ = false;
        return false;
    }
}
```

**Anti-pattern observed** (`serial.cpp:61-64`):
```cpp
catch (...) {
    is_open_ = false;
    return 0;
}
```
The style guide recommends at minimum logging the error, and preferably catching `std::exception` to extract the error message.

## Logging

**Framework:** Custom `tools::Logger` singleton (`tools/logger.hpp`, `tools/logger.cpp`), no external dependencies (uses `std::format` for formatting, C++20)

**Patterns:**
- Macro-based invocation: `LOG_INFO(MODULE, "msg {}", arg)`
- Each `.cpp` defines a module constant: `static constexpr const char* MODULE = "SERIAL";`
- Module names are UPPER_CASE: `SERIAL`, `MAIN`, `TEST`, `FOXGLOVE_COMM`
- Logger initialization at program start via `Logger::instance().init(cfg)`
- `LOG_DEBUG` is compiled out in Release mode (`#ifdef NDEBUG`)
- Levels: `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`
- Format string uses `{}` placeholders (via `std::format`)
- Thread-safe through `std::mutex` in `Logger::log()` (`logger.cpp:47`)

**Init pattern** (`test_logger.cpp:8-15`):
```cpp
tools::LoggerConfig cfg{
    .level = tools::LogLevel::Debug,
    .enable_console = true,
    .enable_file = false,
    .file_path = "logs.txt"
};
tools::Logger::instance().init(cfg);
```

**Usage pattern** (`test_serial.cpp:35-38`):
```cpp
if (!tools::check_crc16(...)) {
    LOG_WARN(MODULE, "CRC check failed");
    return;
}
LOG_INFO(MODULE, "{}", pkt.data);
```

## Comments

**When to Comment:**
- File headers: `// Created by [author] on [date].` present in most files (e.g., `// Created by tgu on 2026/4/14.`)
- Style guide recommends Doxygen-style for public API: `/** @brief @param @return */` (`项目结构与代码风格.md` section 14)
- Inline comments explain "why" not "what"
- `TODO(name): reason` convention recommended for temporary code markers
- C-style comments used sparingly (`项目结构与代码风格.md` recommends adding Doxygen over time)

**JSDoc/TSDoc:**
- Not applicable (C++ project)

## Function Design

**Size:** Functions are generally short and focused:
- `Logger::init()`: ~12 lines (`logger.cpp:25-37`)
- `Serial::open()`: ~26 lines (`serial.cpp:17-42`)
- `Serial::spin_once()`: ~12 lines (`serial.cpp:67-80`)
- `get_crc16()`: ~10 lines (`crc.cpp:50-60`)

**Parameters:**
- References preferred for output: `bool read(cv::Mat& image)` (recommended in style guide)
- `const std::string&` for configuration inputs (`serial.hpp:104`)
- `const uint8_t* data, size_t size` for buffer parameters (`serial.hpp:108`)

**Return Values:**
- `bool` for success/failure operations (`open()`, `is_open()`, `ok()`, `reset()`)
- `size_t` for byte counts (`write()`, `send()`)
- `void` for operations that log errors internally (`spin_once()`)
- `const std::string&` for accessors (`host()`, `port()`)

## Module Design

**Exports:**
- Classes exported via header files in their module's namespace (`io::Serial`, `tools::Logger`, `tools::FoxGloveComm`)
- Free functions in `tools` namespace for utility (`get_crc16()`, `check_crc16()`)
- Template classes fully defined in headers (`RingBuffer<N>`, `StructParser<T, N>` in `serial.hpp`)

**Barrel Files:**
- Not used. Each module has individual headers with no aggregate `io.hpp` or `tools.hpp` include-all.

**PImpl Idiom:**
- `FoxGloveComm` uses the PImpl pattern with `std::unique_ptr<Impl>` (`foxglove_comm.hpp:41-42`)
- Copy deleted, move enabled (`foxglove_comm.hpp:20-27`)

**Template Design:**
- `RingBuffer<N>` and `StructParser<T, N>` are fully defined in `serial.hpp`
- Template static member `Serial::parser_<T>` is defined at namespace scope (`serial.hpp:147`)

## Class Design

**Rule of Five:**
- `FoxGloveComm`: copy deleted, move explicitly defaulted (`= default` in `foxglove_comm.cpp:64-66`)
- `Serial`: copy not explicitly deleted but has raw pointer/member semantics (implicitly deleted due to `boost::asio::serial_port` non-copyable member)
- `Logger`: singleton, copy deleted by making constructor private

**Singleton Pattern:**
- `Logger` uses Meyer's Singleton: `static Logger inst;` in `instance()` (`logger.cpp:14-16`)

## Comparison with Source Project (sp_vision_25)

**Key differences from the project being migrated FROM (`/home/eldwen/sp_vision_25`):**

| Aspect | TGU_Robocore_2027 | sp_vision_25 |
|--------|-------------------|--------------|
| C++ Standard | C++20 | C++17 |
| Brace style | Attach (same line) | Google style (new line) |
| Indentation | 4 spaces | 2 spaces |
| Pointer alignment | Left | Middle |
| Include guard style | `TGU_ROBOCORE_2027_XXX_HPP` | `TOOLS__CRC_HPP` (double underscore) |
| Namespace indent | Indented | Not indented |
| Logger | Custom singleton via macros (`LOG_INFO`) | spdlog via `tools::logger()->info()` |
| Config format | TOML (toml++) | YAML (yaml-cpp) |
| `#pragma once` | Used alongside include guards | Not used (guard only) |
| `.clang-format` | Not present | Present (`Google` style) |
| Copy/move control | Explicit (`FoxGloveComm`) | Not explicitly controlled |
| Function naming | `lower_snake_case` (with some camelCase legacy) | `lower_snake_case` |
| Packed struct attribute | `__attribute__((packed))` | `__attribute__((packed))` (same) |
| Dependency style | Boost.Asio, toml++, custom Logger | OpenVINO, spdlog, yaml-cpp, Eigen3 |

---

*Convention analysis: 2026-05-01*
