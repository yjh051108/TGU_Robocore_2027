# Codebase Structure

**Analysis Date:** 2026-05-01

## Directory Layout

```
TGU_Robocore_2027/
├── CMakeLists.txt                 # Top-level build configuration (CMake 3.16, C++20)
├── README.md                      # Project overview and system dependencies
├── .gitignore                     # Git ignore rules
├── get_lines.sh                   # LOC counting script (excludes build/, foxglove/, tomlpp)
├── 项目结构与代码风格.md            # Comprehensive architecture & convention document (Chinese)
│
├── config/                        # Runtime configuration files (TOML format)
│   └── testconfig.toml            # Example config: camera, game parameters
│
├── app/                           # Business logic / algorithm modules
│   ├── CMakeLists.txt             # Build config (currently empty, commented out from top-level)
│   └── auto_aim/                  # Auto-aim module (stub/placeholder)
│       ├── auto_aim.hpp           # Header stub (empty class)
│       └── auto_aim.cpp           # Implementation stub (just includes header)
│
├── io/                            # Hardware I/O and communication drivers
│   ├── CMakeLists.txt             # Static library build (serial + hikrobot, many modules commented out)
│   ├── serial/                    # Boost.Asio-based serial communication
│   │   ├── serial.hpp             # RingBuffer + StructParser + Serial class (template-heavy, ~150 lines)
│   │   └── serial.cpp             # Serial implementation (open, close, write, spin_once)
│   └── hikrobot/                  # Hikrobot camera interface (stub/placeholder)
│       ├── hikrobot.hpp           # Empty header
│       └── hikrobot.cpp           # Empty implementation
│
├── task/                          # Executable entry points and tests
│   ├── CMakeLists.txt             # Build config for all executables
│   ├── sentry.cpp                 # Main robot entry point (stub/placeholder, empty)
│   └── test/                      # Component-level test programs
│       ├── read_toml.cpp          # TOML config parsing test
│       ├── test_camera.cpp        # Aravis camera acquisition + OpenCV test
│       ├── test_foxglove.cpp      # Foxglove WebSocket test (commented out)
│       ├── test_logger.cpp        # Logger functionality test
│       └── test_serial.cpp        # Serial read/write + CRC validation test
│
└── tools/                         # Reusable utility libraries (no CMakeLists.txt yet)
    ├── crc.hpp                    # CRC8 and CRC16 function declarations
    ├── crc.cpp                    # CRC8 and CRC16 lookup-table implementations
    ├── logger.hpp                 # Logger class + LOG_* macros (header-only interface)
    ├── logger.cpp                 # Logger singleton implementation
    ├── foxglove_comm.hpp          # Foxglove WebSocket server wrapper (PImpl)
    ├── foxglove_comm.cpp          # FoxgloveComm implementation
    ├── tomlpp.hpp                 # toml++ single-header library (third-party)
    └── foxglove/                  # Foxglove SDK (third-party C++ library)
        ├── include/foxglove/      # Public headers: channel, context, server, schema, etc.
        ├── include/foxglove-c/    # C bindings header
        ├── src/                   # SDK implementation files
        │   ├── channel.cpp
        │   ├── context.cpp
        │   ├── foxglove.cpp
        │   ├── mcap.cpp
        │   ├── messages.cpp
        │   ├── server.cpp
        │   └── server/            # Server sub-modules
        │       ├── connection_graph.cpp
        │       ├── fetch_asset.cpp
        │       ├── parameter.cpp
        │       └── service.cpp
        └── lib/                   # Precompiled SDK libraries
            ├── libfoxglove.a
            └── libfoxglove.so

```

## Directory Purposes

**config/:**
- Purpose: Runtime parameter storage. Camera intrinsics, game settings, serial port config, Foxglove settings.
- Contains: `.toml` configuration files with `[section]` organization.
- Key files: `config/testconfig.toml` — example config with `[game]` and `[camera]` sections.
- Convention: Use `value_or()` for defaults; array configs (camera matrix) checked via `as_array()`.

**app/:**
- Purpose: High-level robot algorithms — auto-aim, navigation, target tracking, trajectory prediction, game-state decision making.
- Contains: One subdirectory per algorithm module, each with `.hpp`/`.cpp` pair.
- Key files: `app/auto_aim/auto_aim.hpp` — currently an empty placeholder.
- Convention: Modules depend on `io/` for data and `tools/` for utilities; never depend on `task/`.

**io/:**
- Purpose: Hardware abstraction layer — stable data input/output to physical devices.
- Contains: One subdirectory per device type.
- Key files:
  - `io/serial/serial.hpp` — Core serial abstraction with RingBuffer (circular buffer) and StructParser (template binary protocol parser)
  - `io/serial/serial.cpp` — Boost.Asio port open/close/read/write implementation
  - `io/hikrobot/hikrobot.hpp` — Camera driver stub (Aravis-based, not yet implemented)
- Convention: Each device has `open()`/`close()`/`read()` lifetime; `io/` never imports `app/`.

**task/:**
- Purpose: Executable programs — robot main loop and component tests.
- Contains: `sentry.cpp` (robot entry) and `test/*.cpp` (tests for individual modules).
- Key files:
  - `task/sentry.cpp` — Empty placeholder for main robot control program.
  - `task/test/test_serial.cpp` — Tests serial open/write/read with CRC validation.
  - `task/test/test_camera.cpp` — Demonstrates Aravis camera frame acquisition and OpenCV conversion.
  - `task/test/test_logger.cpp` — Verifies logger format and output.
  - `task/test/read_toml.cpp` — Reads camera matrix and distortion from TOML config.
  - `task/test/test_foxglove.cpp` — Foxglove server test (currently all commented out).
- Convention: `task/test/` is for temporary tests; `task/sentry.cpp` is the production entry.

**tools/:**
- Purpose: Generic reusable utilities with no business logic dependencies.
- Contains: Standalone `.hpp`/`.cpp` files and third-party SDK subdirectory.
- Key files:
  - `tools/logger.hpp` / `.cpp` — Singleton logger with level filtering, console/file output, `MODULE`-based tagging.
  - `tools/crc.hpp` / `.cpp` — CRC8/CRC16 table computation and validation.
  - `tools/foxglove_comm.hpp` / `.cpp` — Foxglove WebSocket server PImpl wrapper.
  - `tools/tomlpp.hpp` — toml++ library (single header, third party).
  - `tools/foxglove/` — Foxglove SDK (headers, sources, prebuilt libraries).
- Convention: Third-party code goes in subdirectories; own wrappers stay at `tools/` root.

## Key File Locations

**Entry Points:**
- `task/sentry.cpp`: Main robot control entry (stub).
- `task/test/test_serial.cpp`: Serial test executable.
- `task/test/test_camera.cpp`: Camera test executable.
- `task/test/test_logger.cpp`: Logger test executable.
- `task/test/read_toml.cpp`: Config parsing test executable.
- `task/test/test_foxglove.cpp`: Foxglove test executable (commented out).

**Configuration:**
- `config/testconfig.toml`: Camera matrix, distortion coefficients, game settings, exposure.
- `CMakeLists.txt`: Build configuration, dependency finding, subdirectory registration.

**Core Logic:**
- `io/serial/serial.hpp`: RingBuffer, StructParser, Serial class (template-heavy serial abstraction).
- `io/serial/serial.cpp`: Boost.Asio serial port implementation.
- `tools/logger.cpp`: Logger singleton with formatting, file/console output.
- `tools/crc.cpp`: CRC8/CRC16 lookup table calculation.
- `tools/foxglove_comm.cpp`: Foxglove WebSocket server PImpl wrapper.

**Testing:**
- `task/test/test_serial.cpp`: Serial + CRC round-trip validation.
- `task/test/test_logger.cpp`: Logger output verification.
- `task/test/test_camera.cpp`: Aravis camera acquisition test.
- `task/test/read_toml.cpp`: TOML parsing test.
- `task/test/test_foxglove.cpp`: Foxglove server test (commented out).

## Naming Conventions

**Files:**
- `lower_snake_case` for all source and header files: `serial.cpp`, `foxglove_comm.hpp`, `test_logger.cpp`.
- `.hpp` for C++ headers, `.cpp` for source files.
- `.toml` for configuration files.
- Third-party files follow their own conventions (Foxglove SDK uses `snake_case`).

**Directories:**
- `lower_snake_case` for all directories: `auto_aim`, `foxglove_comm`, `hikrobot`.
- One directory per device/algorithm module.

**Namespaces:**
- `namespace io {}` for hardware I/O (`io/serial/`, `io/hikrobot/`).
- `namespace tools {}` for utilities (`tools/logger.hpp`, `tools/crc.hpp`).
- `namespace app {}` (recommended) for business logic modules.

**Classes:**
- `UpperCamelCase`: `Serial`, `Logger`, `RingBuffer`, `StructParser`, `FoxGloveComm`, `LoggerConfig`.

**Functions:**
- `lower_snake_case`: `is_open()`, `spin_once()`, `get_crc16()`, `check_crc16()`.

**Variables:**
- `lower_snake_case` for local and member variables.
- Trailing underscore `_` for class member variables: `level_`, `console_`, `file_`, `mutex_`.
- `UPPER_SNAKE_CASE` for constants: `HEAD_SIZE`, `CRC16_INIT`, `MODULE`.

**Types:**
- `UpperCamelCase` for structs, enums, and type aliases: `LogLevel`, `RecvPackage`, `SendPackage`.

## Where to Add New Code

**New Feature (e.g., auto-aim algorithm):**
- Primary code: `app/auto_aim/` — one `.hpp`/`.cpp` pair per class, e.g., `detector.hpp/cpp`, `tracker.hpp/cpp`.
- Config: `config/camera.toml`, `config/game.toml`, `config/serial.toml`.
- Entry: Wire into `task/sentry.cpp` main loop.
- Tests: `task/test/test_auto_aim.cpp`.

**New Hardware Driver (e.g., IMU):**
- Implementation: `io/imu/imu.hpp` and `io/imu/imu.cpp`.
- Inherit from or compose with existing io patterns.
- Register in `io/CMakeLists.txt` as source file.
- Test: `task/test/test_imu.cpp`.

**New Utility (e.g., Kalman filter):**
- Implementation: `tools/extended_kalman_filter.hpp` and `tools/extended_kalman_filter.cpp`.
- Test: `task/test/test_kalman.cpp`.

**New Entry Point:**
- File: `task/sentry.cpp` for main robot; `task/test/test_xxx.cpp` for tests.
- Register in `task/CMakeLists.txt` with `add_executable()` + `target_link_libraries()`.

## Special Directories

**tools/foxglove/:**
- Purpose: Foxglove WebSocket visualization SDK (third-party).
- Generated: No (checked-in SDK library and source).
- Committed: Yes.
- Contains: Precompiled `.a`/`.so` libraries, C++ headers, implementation source files.
- Note: The custom Foxglove wrapper lives at `tools/foxglove_comm.hpp/cpp`, separate from the SDK.

**task/test/:**
- Purpose: Temporary component-level test programs, not formal unit tests.
- Generated: No.
- Committed: Yes.
- Contains: Small `main()` functions that exercise a single module.
- Note: Test files may contain experimental/path-specific code (hardcoded device paths, etc.).

---

## Cross-Project Directory Comparison: sp_vision_25 vs TGU_Robocore_2027

| sp_vision_25 (Source) | TGU_Robocore_2027 (Target) | Notes |
|---|---|---|
| `src/*.cpp` (12 entry points) | `task/sentry.cpp` + `task/test/*.cpp` | Consolidates scattered entries into `task/` |
| `tasks/auto_aim/` | `app/auto_aim/` | Renamed for clarity; same purpose |
| `tasks/auto_buff/` | `app/auto_buff/` (planned) | Not yet migrated |
| `tasks/omniperception/` | `app/decision/` (planned) | Not yet migrated |
| `io/serial/` (serial:Serial) | `io/serial/` (io::Serial via Boost.Asio) | Different serial library; same directory name |
| `io/hikrobot/` (vendor SDK) | `io/hikrobot/` (Aravis) | Same directory, different camera backend |
| `io/mindvision/` | Not present | Not yet migrated |
| `io/usbcamera/` | Not present | Not yet migrated |
| `io/dm_imu/` | Not present | Not yet migrated |
| `io/gimbal/` | Not present | Replaced by generalized `serial/` protocol |
| `io/ros2/` | Not present | ROS2 support not planned |
| `io/camera.cpp/hpp` (Camera wrapper) | Not present | Not yet migrated |
| `io/cboard.cpp/hpp` (Mainboard IF) | Not present | Not yet migrated |
| `io/socketcan.hpp` | Not present | Not yet migrated |
| `tools/` (OBJECT lib, 15 files) | `tools/` (no CMake target, 5 files) | TGU started fresh with subset of tools |
| `configs/` (10 YAML files) | `config/` (1 TOML file) | Switched format YAML -> TOML |
| `tests/` (22 test executables) | `task/test/` (5 test executables) | Fewer tests carried over |
| `calibration/` (5 programs) | Not present | Not yet migrated |
| `assets/` (model files) | Not present | Not yet migrated |
| `autostart.sh`, `watchdog.sh` | Not present | Not yet migrated |

---

*Structure analysis: 2026-05-01*
