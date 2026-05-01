<!-- refreshed: 2026-05-01 -->
# Architecture

**Analysis Date:** 2026-05-01

## System Overview

```text
┌──────────────────────────────────────────────────────────────────┐
│                        task/ (Entry Points)                       │
│   `task/sentry.cpp`        `task/test/*.cpp`                      │
└────────────┬────────────────────────┬─────────────────────────────┘
             │                        │
             ▼                        ▼
┌─────────────────────┐  ┌──────────────────────────────────────┐
│    app/ (Business)   │  │     tools/ (Utility Infrastructure)  │
│  `app/auto_aim/`     │  │  `tools/logger.hpp`                  │
│  `app/navigation/`   │  │  `tools/crc.hpp`                     │
│                      │  │  `tools/foxglove_comm.hpp`           │
│                      │  │  `tools/tomlpp.hpp`                  │
│                      │  │  `tools/foxglove/` (3rd-party SDK)   │
└──────────┬───────────┘  └────────────────────┬─────────────────┘
           │                                    │
           ▼                                    ▼
┌──────────────────────────────────────────────────────────────────┐
│                        io/ (Hardware & Communication)             │
│  `io/serial/`  (Boost.Asio-based struct-driven serial protocol)  │
│  `io/hikrobot/` (Aravis-based Hikrobot camera, placeholder)      │
└──────────────────────────────────────────────────────────────────┘
```

### Data Flow Direction

```
config/  -->  task/  -->  app/
               |           |
               v           v
              io  <----> tools
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| `io::Serial` | Boost.Asio-based serial port with template-based struct parsing and ring buffer | `io/serial/serial.hpp` |
| `io::StructParser` | Template-based binary protocol parser, frame header matching, deserialization | `io/serial/serial.hpp:48-96` |
| `io::RingBuffer` | Circular byte buffer for anti-stick/unpack, fixed-size template | `io/serial/serial.hpp:18-44` |
| `tools::Logger` | Singleton logging with console/file output, level filtering, log macros | `tools/logger.hpp` |
| `tools::CRC` | CRC8/CRC16 table-based calculation and validation | `tools/crc.hpp` |
| `tools::FoxGloveComm` | Foxglove WebSocket server wrapper for real-time visualization | `tools/foxglove_comm.hpp` |
| `tools::tomlpp` | TOML config file parsing (single-header library) | `tools/tomlpp.hpp` |
| `app::auto_aim` | Auto-aim module (stub/placeholder, not yet implemented) | `app/auto_aim/` |
| `task/` entry points | Executable main() functions that wire config, io, tools, and app together | `task/sentry.cpp`, `task/test/*.cpp` |

## Pattern Overview

**Overall:** Modular monolith with layered architecture, strict dependency direction.

**Key Characteristics:**
- Each directory is a self-contained module with its own `CMakeLists.txt`
- Dependency rule: `config > task > app > io <-> tools` (io and tools are peers)
- Hardware drivers in `io/` never depend on business logic in `app/`
- Utility code in `tools/` never depends on `io/`, `app/`, or `task/`
- Entry points in `task/` are thin wiring layers: create objects, run main loop, clean up
- Templates are used extensively in `io::Serial` for type-safe binary protocol parsing
- Configuration via TOML files in `config/`, read through `tomlpp.hpp`

## Layers

**config Layer:**
- Purpose: Runtime parameters and calibration data (camera intrinsics, serial config, game settings)
- Location: `config/`
- Contains: `.toml` configuration files
- Depends on: Nothing (pure data)
- Used by: All other layers

**io Layer (Hardware Abstraction):**
- Purpose: Stable data acquisition and command output to hardware
- Location: `io/`
- Contains: Serial port driver, camera drivers (Hikrobot placeholder), device abstractions
- Depends on: `tools/` (for logger), Boost.Asio, Aravis
- Used by: `task/`, `app/`
- Key design: Each device gets its own subdirectory under `io/`

**tools Layer (Utility):**
- Purpose: Reusable utility code not tied to any robot task
- Location: `tools/`
- Contains: Logger, CRC, Foxglove wrapper, toml++ parser, Foxglove SDK
- Depends on: System libraries only
- Used by: `io/`, `app/`, `task/`
- Key design: Should be independently testable, third-party libraries in subdirectories

**app Layer (Business Logic):**
- Purpose: Robot-specific algorithms (auto-aim, navigation, tracking, decision)
- Location: `app/`
- Contains: `auto_aim/` (placeholder), navigation, tracker, predictor, decision
- Depends on: `io/`, `tools/`
- Used by: `task/`
- Key design: Input/output contracts should be clear for algorithm swapping

**task Layer (Entry Points):**
- Purpose: Wiring together config, hardware, and algorithms into executable programs
- Location: `task/`
- Contains: `sentry.cpp` (main robot entry), `test/*.cpp` (component tests)
- Depends on: Every other layer
- Used by: End-user execution

## Data Flow

### Primary Request Path (Serial Communication)

1. Application code calls `serial.send<T>(data)` to transmit a packed struct (`io/serial/serial.hpp:111-113`)
2. `serial.write()` delegates to `boost::asio::write()` (`io/serial/serial.cpp:56-65`)
3. Received bytes arrive asynchronously, read via `serial.spin_once()` (`io/serial/serial.cpp:67-80`)
4. Raw bytes passed through registered callbacks; `StructParser<T>::input()` demuxes frames (`io/serial/serial.hpp:56-81`)
5. Full parsed struct delivered to callback with CRC validation by caller (`task/test/test_serial.cpp:33-38`)

### Configuration Read Path

1. `task/test/read_toml.cpp` demonstrates: `toml::parse_file("path")` (`task/test/read_toml.cpp:12`)
2. Values extracted via `config["section"]["key"].value_or(default)` pattern
3. Camera matrix read as array, distortion coefficients as array

### Startup Initialization Sequence

1. Entry point `main()` parses command-line args and config path
2. Logger initialized via `tools::Logger::instance().init(cfg)` (`task/test/test_logger.cpp:8-15`)
3. Hardware drivers constructed (serial, camera)
4. Application modules initialized
5. Main loop: read sensors, process, command actuators
6. Clean shutdown on exit

**State Management:**
- `tools::Logger` is a global singleton (`tools/logger.hpp:33`)
- `io::Serial` is an owned object with RAII open/close lifecycle
- Module-level `static` state in `Serial::parser_<T>` template — note: this is a shared static across all `Serial` instances for a given type T

## Key Abstractions

**StructParser<T>:**
- Purpose: Generic binary protocol parser that matches frame headers and deserializes packed structs
- Location: `io/serial/serial.hpp:48-96`
- Pattern: Template with frame-head matching, ring-buffer backed state machine
- Analogy: An abstraction for "receive typed packets from byte stream" without per-protocol boilerplate

**RingBuffer<N>:**
- Purpose: Fixed-size circular byte buffer for serial data accumulation
- Location: `io/serial/serial.hpp:18-44`
- Pattern: Template parameter for buffer size, O(1) push/pop/at operations
- Used to prevent serial frame desync and handle partial reads

**FoxGloveComm (PImpl):**
- Purpose: RAII wrapper for Foxglove WebSocket server
- Location: `tools/foxglove_comm.hpp`
- Pattern: PImpl (Pointer to Implementation) for hiding third-party SDK details
- Non-copyable, movable, with `ok()` status query

**Logger (Singleton):**
- Purpose: Centralized logging with level filtering, console/file output
- Location: `tools/logger.hpp`
- Pattern: Meyer's singleton, macro-based convenience wrappers (`LOG_INFO`, `LOG_DEBUG`, etc.)
- Debug logging stripped at compile time in Release builds

## Entry Points

**Sentry Main Entry:**
- Location: `task/sentry.cpp`
- Triggers: Robot startup (manual launch)
- Responsibilities: Main robot control loop (placeholder/stub)

**Test Executables:**
- `task/test/test_serial.cpp`: Serial communication + CRC round-trip test
- `task/test/test_logger.cpp`: Logger functionality test
- `task/test/test_camera.cpp`: Aravis camera acquisition + OpenCV conversion
- `task/test/read_toml.cpp`: TOML config parsing demonstration
- `task/test/test_foxglove.cpp`: Foxglove WebSocket test (commented out)

## Architectural Constraints

- **Threading:** Currently single-threaded by design (`Serial::spin_once()` is synchronous). No thread pool or concurrent processing infrastructure yet.
- **Global state:** `tools::Logger::instance()` is a global singleton (`tools/logger.cpp:14-17`). `io::Serial::parser_<T>` is a static template member shared across all Serial instances (`io/serial/serial.hpp:147`).
- **Circular imports:** Not detected — dependency direction is enforced by project convention.
- **Template in header:** All template code (RingBuffer, StructParser) must be in headers.
- **Library structure:** `io/` is built as a STATIC library; `tools/` does not yet have its own `CMakeLists.txt`.

## Architectural Differences from sp_vision_25 (Source Project)

The TGU_Robocore_2027 project is a ground-up redesign migrating from the sp_vision_25 codebase. Key architectural differences:

| Aspect | sp_vision_25 (Source) | TGU_Robocore_2027 (Target) |
|--------|-----------------------|---------------------------|
| **C++ Standard** | C++17 | C++20 |
| **Config Format** | YAML (yaml-cpp) | TOML (toml++) |
| **Config Style** | `tools::load()` + `tools::read<T>()` with exit-on-error | `toml::parse_file()` + `value_or(defaults)` |
| **Serial Library** | Boost serial (`serial::Serial`) via external lib | Boost.Asio (`boost::asio::serial_port`) |
| **Serial Protocol** | Fixed structs (gimbal-specific, hardcoded) | Template-based generic `StructParser<T>` |
| **Logger** | spdlog (third-party) | Custom singleton (`tools::Logger`) |
| **Neural Network** | OpenVINO (Intel, model.xml format) | Not yet implemented |
| **Camera SDK** | Hikrobot/Mindvision vendor SDKs + USBCamera | Aravis (GenICam generic, `arv_*` API) |
| **Visualization** | Raw UDP socket plotter (port 9870) | Foxglove WebSocket (port 8765) |
| **Entry Points** | Multiple in `src/` (standard, sentry, uav, debug variants) | Single `task/sentry.cpp` + test executables in `task/test/` |
| **Multi-threading** | Thread pool, thread-safe queues, parallel detection | Single-threaded (not implemented yet) |
| **MPC Planner** | tinyMPC-based trajectory planning (`tinympc/`) | Not yet implemented |
| **IMU** | DM IMU via serial protocol + `DM_IMU` driver | Not yet implemented |
| **CAN Bus** | SocketCAN with epoll, daemon thread, auto-reconnect | Not yet implemented |
| **ROS2** | Optional ROS2 publishing (sentry, navigation msgs) | Not planned |
| **Directory Structure** | Flat `src/`, `tasks/` with sub-task modules | `task/` for entries, `app/` for business logic |
| **Tool library** | Object library (`add_library(OBJECT)`) | Not yet implemented as target |
| **Ceres solver** | Required for auto_buff optimization | Not present |
| **Eigen3** | Required for all geometry | Not currently linked (commented out in CMakeLists.txt) |

**Migration Strategy Observations:**
- The new architecture is intentionally simpler and more modular
- The template-based StructParser in `io/serial/` is a generalization of what was hardcoded in sp_vision_25's `io/gimbal/gimbal.cpp`
- The Foxglove integration replaces the raw UDP plotter with a proper WebSocket visualization server
- Aravis replaces vendor-specific camera SDKs for cross-camera compatibility
- The `app/` directory pattern mirrors `tasks/` but with clearer naming
- The `task/` directory consolidates what was spread across `src/` and `tests/` in sp_vision_25

## Anti-Patterns

### Static Template Member Sharing

**What happens:** `io::Serial::parser_<T>` is a `static` member template in `io/serial/serial.hpp:147`, meaning ALL instances of `Serial` share the same parser for a given type `T`.
**Why it's wrong:** If two `Serial` instances (e.g., two serial ports) both receive the same struct type, their byte streams will corrupt each other's parser state.
**Do this instead:** Make parser state per-instance, not per-type. Move `parser_<T>` from `static` to a member variable (use `std::map<std::type_index, ...>` or similar).

### Catch-All Exception Handling

**What happens:** `io/serial/serial.cpp:61-63` and `io/serial/serial.cpp:76-78` use `catch (...)` with no logging, just setting `is_open_ = false` and returning zero.
**Why it's wrong:** Exceptions are silently swallowed, making debugging serial failures extremely difficult.
**Do this instead:** Log the exception before returning, even in a catch-all:
```cpp
catch (const std::exception& e) {
    LOG_ERROR(MODULE, "write failed: {}", e.what());
    is_open_ = false;
    return 0;
} catch (...) {
    LOG_ERROR(MODULE, "write failed: unknown exception");
    is_open_ = false;
    return 0;
}
```

### Missing tools/CMakeLists.txt

**What happens:** The top-level `CMakeLists.txt` does `add_subdirectory(tools)` but `tools/` has no `CMakeLists.txt`, so the target has no effect.
**Why it's wrong:** The build system is incomplete; `tools` source files are not compiled as a library. Currently the `io` library links directly against system libs but has no assured `tools` target.
**Do this instead:** Add a `tools/CMakeLists.txt` defining the `tools` library target with its source files.

## Error Handling

**Strategy:** Return-boolean-with-log approach for device operations. Serial `open()` returns `bool` and logs success/failure (`io/serial/serial.cpp:17-43`). Write returns `0` on failure.

**Patterns:**
- Device open: return `bool`, log `LOG_INFO` on success, `LOG_ERROR` on failure
- Device write: return `size_t`, return `0` on failure, set `is_open_ = false`
- CRC validation: caller must check via `check_crc16()` before using data
- Config parsing: use `value_or()` for defaults, no hard failure on missing key

**Gaps:**
- `catch (...)` blocks do not log errors
- No error propagation from serial read failure to application layer
- No timeout mechanism in `spin_once()` for application-level watchdog

## Cross-Cutting Concerns

**Logging:** Custom `tools::Logger` singleton with `LOG_INFO/DEBUG/WARN/ERROR` macros. Module name defined per-file as `static constexpr const char* MODULE = "NAME"`. (`tools/logger.hpp`)

**Validation:** CRC8/CRC16 at application level, no transport-level validation. Serial protocol relies on frame header matching for synchronization. (`tools/crc.hpp`)

**Configuration:** TOML files via `tomlpp.hpp`. Paths relative to build directory (`../../config/`). Pattern: `config["section"]["key"].value_or(default)`. (`tools/tomlpp.hpp`, `config/testconfig.toml`)

**Platform Constraints:** Currently Linux-only (Boost.Asio), no Windows support planned. Aravis camera drivers are Linux/Unix only.

---

*Architecture analysis: 2026-05-01*
