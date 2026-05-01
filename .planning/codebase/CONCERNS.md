# Codebase Concerns

**Analysis Date:** 2026-05-01

## Tech Debt

### Serial Static Parser Design Flaw

- Issue: `Serial::parser_<T>` is a static template member (`io/serial/serial.hpp:142`), meaning ALL `Serial` instances that use the same type T share the same `StructParser<T>`. If multiple serial ports are opened or multiple `recv()` callbacks with the same type are registered, their byte streams will be mixed in the same ring buffer, corrupting all parsing state.
- Files: `io/serial/serial.hpp:142-147`
- Impact: Using more than one Serial instance with the same packet type causes data corruption and packet desync. This prevents multi-serial-port robot configurations (e.g., separate serial for gimbal, chassis, and裁判系统).
- Fix approach: Move `parser_<T>` from `static` to per-instance storage. Options: (a) use `std::unordered_map<std::type_index, StructParser<T>>` as a member variable, (b) store the parser inside the callback wrapper lambda, or (c) refactor `recv()` to bind a parser instance per registration.

### Stub Modules (Empty Placeholder Files)

- Issue: Several modules exist only as stub files with include guards and nothing else, giving a false sense of completion.
- Files:
  - `app/auto_aim/auto_aim.hpp` -- 8 lines (guard only)
  - `app/auto_aim/auto_aim.cpp` -- 5 lines (empty include)
  - `io/hikrobot/hikrobot.hpp` -- 10 lines (guard only)
  - `io/hikrobot/hikrobot.cpp` -- 5 lines (empty include)
  - `task/sentry.cpp` -- 3 lines (comment only)
  - `app/CMakeLists.txt` -- 1 line (empty)
- Impact: Misleading project structure; no actual functionality behind these directories. The planned Phase 1-9 auto_aim migration (39 requirements) has zero code delivered. Sentry entry point is non-functional.
- Fix approach: Implement modules or remove stub files to avoid confusion.

### CMake Configuration Gaps

- Issue: `io/CMakeLists.txt:18` has `# tools` commented out in `target_link_libraries(io ...)`, but `io/serial/serial.cpp` includes `<tools/logger.hpp>` and calls `LOG_INFO`, `LOG_ERROR` macros. This means `io` depends on `tools` symbols but does not declare the link dependency, which will cause linker errors when building the `io` static library into executables that don't also explicitly link `tools`.
- File: `io/CMakeLists.txt:16-21`
- Impact: Build failures on certain linker configurations or when modules are reorganized. Currently masked because test executables (`test_serial`) explicitly link both `io` and `tools`.
- Fix approach: Uncomment `tools` in `io/CMakeLists.txt` `target_link_libraries(io PUBLIC tools)`.

### Missing tools/CMakeLists.txt

- Issue: The top-level `CMakeLists.txt` calls `add_subdirectory(tools)`, but there is no `tools/CMakeLists.txt` to define the `tools` library target. The tools library is currently built only through the `task/CMakeLists.txt` targets that directly compile source files, or not at all via a proper library target.
- File: `tools/` directory
- Impact: The `tools` directory is included as a subdirectory but has no build target. The `FoxGloveComm` constructor re-initializes the Logger (`foxglove_comm.cpp:58`) because there is no guarantee of prior initialization order.
- Fix approach: Add `tools/CMakeLists.txt` with `add_library(tools ...)` and appropriate source files. This was already identified as priority #1 in the project style guide.

### test_foxglove.cpp Fully Commented Out

- Issue: `task/test/test_foxglove.cpp` has its entire main body commented out. The test exists as a file but does nothing.
- File: `task/test/test_foxglove.cpp:5-18`
- Impact: No automated verification of Foxglove WebSocket server functionality.
- Fix approach: Either implement a working test, or remove the file.

### read_toml Test Missing Link Dependencies

- Issue: `task/CMakeLists.txt:2` defines `add_executable(read_toml test/read_toml.cpp)` but does not have any `target_link_libraries(read_toml ...)`. The test uses `toml++` directly but without explicit link to `tools` (which technically doesn't exist as a library target either).
- File: `task/CMakeLists.txt:2`
- Impact: Compilation may fail when build system is cleaned/reorganized, or when toml++ internal dependencies change.
- Fix approach: Add `target_link_libraries(read_toml PRIVATE tools)` after tools/CMakeLists.txt is created.

### Commented-Out io Submodules

- Issue: `io/CMakeLists.txt:7-13` has 5 commented-out source files referencing old sp_vision architecture (`mindvision`, `usbcamera`, `cboard`, `dm_imu`, `gimbal`). These suggest an intention to port modules from sp_vision but no progress has been made.
- File: `io/CMakeLists.txt:7-13`
- Impact: Dead configuration code that may confuse developers about the intended io layer structure.
- Fix approach: Either remove commented-out lines, or create an issue to port these modules.

### Logger Re-initialization Risk

- Issue: `FoxGloveComm` constructor (`foxglove_comm.cpp:58`) calls `Logger::instance().init(impl_->cfg)` with a hardcoded config. If `Logger::init()` was already called elsewhere (e.g., in `main()` or another module), this silently overwrites the existing config. The `init()` method has no guard against double-initialization.
- File: `tools/foxglove_comm.cpp:21-25,58`
- Impact: Logger config set by the application's `main()` can be silently overwritten by `FoxGloveComm` construction order, causing unexpected log level or output changes.
- Fix approach: Add a guard in `Logger::init()` to skip if already initialized, or remove the `init()` call from `FoxGloveComm` constructor and require callers to initialize Logger first.

## Known Bugs

### CRC16 Endianness Assumption

- Symptoms: `check_crc16()` in `tools/crc.cpp:83` reads the CRC bytes from the byte stream as big-endian: `(data[len-1] << 8) | data[len-2]`. On big-endian architectures (uncommon for Robocore target platforms), this will produce incorrect CRC comparison. On little-endian (x86_64, aarch64), the packed struct layout stores uint16_t as `[low_byte, high_byte]`, which when read back as big-endian reconstructs the correct value. This works by coincidence on current target platforms but is not portable.
- Files: `tools/crc.cpp:82-84`
- Trigger: Compiling for or running on a big-endian platform.
- Workaround: Current targets (x86_64, aarch64) are both little-endian, so this is a latent portability issue rather than an active bug. Document the endianness assumption explicitly.

### Empty catch(...) Blocks Silently Swallow Errors

- Symptoms: `serial.cpp:61-64` and `serial.cpp:77-79` use bare `catch(...)` blocks that silently set `is_open_ = false` and return 0 without any logging. Any exception (including programming errors like bad_alloc or assertion failures) is silently swallowed.
- Files: `io/serial/serial.cpp:61,77`
- Trigger: Any unexpected exception during serial write or spin_once.
- Workaround: None. Errors are silently hidden.
- Fix approach: Change to `catch (const std::exception& e)` with `LOG_ERROR`, and keep a `catch(...)` only for truly unknown errors with a log message as specified in the project style guide.

### Camera Test Uses const_cast on Aravis Buffer Data

- Symptoms: `test_camera.cpp:81` uses `const_cast<void*>(data)` to remove const from the Aravis buffer data pointer when constructing a `cv::Mat`. This is technically undefined behavior if the buffer is in read-only memory.
- File: `task/test/test_camera.cpp:81`
- Trigger: Any run of the camera test on a platform that enforces read-only buffer memory.
- Fix approach: Use `cv::Mat::clone()` to copy the buffer data after wrapping it, or use `cv::Mat(const_cast<void*>(data), ...)` only after confirming the Aravis API guarantees writable buffers. Better yet, encapsulate this in the HikRobot driver.

## Security Considerations

### Device Path Hardcoding

- Risk: `test_serial.cpp:31` hardcodes `/dev/ttyACM0` as the serial device path. In production, this should be configurable via TOML configuration or command-line argument.
- Files: `task/test/test_serial.cpp:31`
- Current mitigation: Test file only — but `task/sentry.cpp` is also empty, so no production entry point exists yet.
- Recommendations: All device paths (serial, camera) should be read from `config/*.toml` files, not hardcoded. The project style guide already recommends this.

### Foxglove Server Exposes Port 8765 on All Interfaces

- Risk: `FoxGloveComm` default host is `0.0.0.0` (`foxglove_comm.hpp:16`), binding the WebSocket server to all network interfaces. On a competition robot, this may expose internal telemetry data to other devices on the same network.
- Files: `tools/foxglove_comm.hpp:16`
- Current mitigation: Default host `0.0.0.0` allows any network connection. Foxglove typically operates on the competition LAN.
- Recommendations: Document that the Foxglove server binds to all interfaces. Consider adding an authentication mechanism or restricting to localhost by default with an option to expose.

## Performance Bottlenecks

### tomlpp.hpp Single Header (17,889 Lines)

- Problem: The `tomlpp.hpp` single-header library (17,889 lines, 63MB for the foxglove SDK) is a massive compilation dependency for every translation unit that includes it. It contains platform-specific pragmas for MSVC, GCC, and Clang.
- Files: `tools/tomlpp.hpp`
- Cause: Single-header distribution model of the toml++ library.
- Improvement path: Use precompiled headers or only include `tomlpp.hpp` in `.cpp` files that need TOML parsing, never in headers. Consider using a smaller TOML parser if compilation time becomes an issue.

### Foxglove SDK Binary Bloat

- Problem: The `tools/foxglove/` directory includes a prebuilt static library (`.a`), shared library (`.so`), 33 header files, and ~4,236 lines of SDK source code. This adds ~63MB to the repository and significant compile time.
- Files: `tools/foxglove/`
- Cause: Bundled third-party SDK with both source and prebuilt binaries.
- Improvement path: Consider using Foxglove as a system-installed package or git submodule rather than bundling binaries. The prebuilt `.a` and `.so` files in git are not ideal for cross-platform builds.

## Fragile Areas

### Serial::recv Template with Static Parser (CRITICAL)

- Files: `io/serial/serial.hpp:115-128, 142-147`
- Why fragile: The `static StructParser<T> parser_` template member makes the entire serial framework fundamentally broken for multi-device use. Any new developer adding a second Serial instance will encounter non-deterministic data corruption.
- Safe modification: Before adding any multi-serial-port feature, refactor `parser_<T>` to be per-instance. The callback lambda wrapper in `recv()` already captures `this`, so storing a per-callback parser in the lambda itself is the simplest fix.
- Test coverage: No tests exercise multi-Serial or multi-callback scenarios.

### Circular Build Dependency Risk Between io and tools

- Files: `io/serial/serial.cpp` includes `tools/logger.hpp` but `io/CMakeLists.txt` does not link `tools`
- Why fragile: This currently "works" because the test executables link both, but any reorganization of the CMake system or addition of a new target that links only `io` will fail at link time.
- Safe modification: Always add `target_link_libraries(io PUBLIC tools)` when modifying `io/CMakeLists.txt`.

### Auto Aim Migration Dependency Chain

- Files: `app/auto_aim/` (entire module), `PROJECT.md`
- Why fragile: The auto_aim migration depends on 9 sequential phases with 39 requirements. Each phase depends on the previous (`ROADMAP.md`). Any break in the chain (e.g., failing to migrate math_tools first) blocks all downstream work. The migration also requires adding Eigen3 as a dependency (sp_vision_25 uses it extensively, Robocore currently does not have `find_package(Eigen3)` enabled).
- Safe modification: Begin with Phase 1 (math_tools) and verify each phase independently before proceeding. Ensure Eigen3 is available in the build environment.
- Test coverage: No auto_aim tests exist yet (planned for Phase 9).

### Camera Module Uses Manual GLib Resource Management

- Files: `task/test/test_camera.cpp:32-108`
- Why fragile: The camera test manually manages `GError*`, `ArvCamera*`, `ArvStream*`, and `ArvBuffer*` GLib objects with explicit `g_object_unref()` calls. There are at least 4 early-return paths, and any missed `g_object_unref()` on an error path causes a memory leak. Some error paths (e.g., line 33) may leak `GError` when both the error pointer is non-null AND camera is null.
- Safe modification: Encapsulate into RAII wrapper classes in `io/hikrobot/`. The style guide already recommends this.

## Scaling Limits

### Single-threaded spin_once Architecture

- Current capacity: The `Serial::spin_once()` method (`serial.cpp:67-79`) reads bytes and dispatches all callbacks synchronously in a single-threaded loop. This works for 1000Hz+ serial streams currently.
- Limit: If multiple high-bandwidth devices are added (e.g., two serial ports at 921600 baud + camera at 60fps), the synchronous `spin_once` model will become a bottleneck. There is no threading, thread pool, or asynchronous I/O in the current codebase (beyond Boost.Asio's synchronous API usage).
- Scaling path: Move to asynchronous Boost.Asio reads with completion handlers, or use a dedicated I/O thread per device with thread-safe queues (similar to sp_vision_25's `thread_safe_queue.hpp` and camera thread model).

## Dependencies at Risk

### Missing Eigen3 Dependency

- Risk: sp_vision_25 uses Eigen3 extensively (`ExtendedKalmanFilter`, `math_tools`, `Trajectory`). The planned migration (PROJECT.md) will require Eigen3, but Robocore's `CMakeLists.txt:19` has `#find_package(Eigen3 REQUIRED)` commented out. Eigen3 is not installed via `README.md`'s apt instructions.
- Impact: Phase 1-7 of the migration plan will fail to compile without Eigen3.
- Migration plan: Uncomment `find_package(Eigen3 REQUIRED)` in top-level `CMakeLists.txt` and add `sudo apt install libeigen3-dev` to README.md. This should be done before starting Phase 1.

### Config Format Mismatch (YAML vs TOML)

- Risk: sp_vision_25 uses YAML for configuration (`yaml-cpp`). Robocore uses TOML (`toml++`). All auto_aim configuration files will need to be recreated in TOML format during Phase 8 (CFG-01).
- Impact: Cannot directly copy sp_vision_25 config files. Every parameter from `yaml` files must be manually translated to `*.toml`.
- Migration plan: Create `config/auto_aim.toml` with equivalent parameters during Phase 8. Use toml++ `value_or()` defaults as specified in the style guide.

### No spdlog/fmt Equivalent in Robocore

- Risk: sp_vision_25 uses `spdlog` and `fmt` extensively. Robocore uses a custom `Logger` with `std::format` (C++20). `std::format` supports `{}` placeholders but does not support `{:.2f}` format specifiers or custom type formatters that `fmt` library provides. Some sp_vision debug formatting may not be directly portable.
- Impact: Format strings in migrated code may need adjustment. The custom Logger does not support spdlog's built-in log level filtering per module or async logging.
- Migration plan: Use Robocore's Logger macros (`LOG_INFO`, `LOG_DEBUG`, etc.) and adjust format strings. For complex formatting, fall back to `std::ostringstream`.

## Missing Critical Features

### No Entry Point (sentry.cpp is Empty)

- Problem: `task/sentry.cpp` is 3 lines of comments. There is no main entry point for the robot application. All executables are test programs in `task/test/`.
- Blocks: Cannot run any integrated system test. The auto_aim pipeline cannot be deployed on the robot.
- Priority: High — the style guide lists this as priority #4 on the recommended cleanup list.

### No Eigen3 Support

- Problem: Eigen3 is not enabled in the build system and not listed in README.md dependencies.
- Blocks: The entire auto_aim migration (Phases 1-9) depends on Eigen3 for linear algebra operations in EKF, math_tools, and trajectory solving.
- Priority: Critical for migration success.

### No Formatter Configuration

- Problem: No `.clang-format` file exists in the repository despite the style guide (`项目结构与代码风格.md:524-539`) providing a detailed recommended configuration. There is no `.clang-tidy` or CI linting either. Code style consistency is entirely manual.
- Blocks: Inconsistent formatting as new code is added during migration.
- Priority: Medium — the style guide lists this as priority #5.

## Test Coverage Gaps

### Untested Area: Serial Module

- What's not tested: No tests for multi-callback registration, static parser contention, or high-throughput scenarios. `test_serial.cpp` only tests a single `RecvPackage` type on a single Serial instance, which does not surface the `static StructParser<T>` design flaw.
- Files: `io/serial/serial.hpp`, `io/serial/serial.cpp`
- Risk: The static parser bug will go unnoticed until someone uses two serial ports.
- Priority: High

### Untested Area: Camera Driver

- What's not tested: No HikRobot camera driver exists. `test_camera.cpp` is a procedural test with raw Aravis API calls. No unit tests for camera configuration, error recovery, or resource cleanup.
- Files: `io/hikrobot/hikrobot.hpp`, `io/hikrobot/hikrobot.cpp`, `task/test/test_camera.cpp`
- Risk: Aravis API calls scattered in test code will be copied into production code, perpetuating manual GLib resource management.
- Priority: Medium

### Untested Area: CRC Module

- What's not tested: No explicit tests for `get_crc8`, `check_crc8`, `get_crc16`, or `check_crc16`. The CRC functions are only tested indirectly through `test_serial.cpp`.
- Files: `tools/crc.hpp`, `tools/crc.cpp`
- Risk: The endianness assumption in `check_crc16` is not validated by any test.
- Priority: Medium

### Untested Area: Foxglove Communication

- What's not tested: `test_foxglove.cpp` is fully commented out. Zero test coverage for Foxglove WebSocket server setup, channel creation, message publishing, or connection lifecycle.
- Files: `tools/foxglove_comm.hpp`, `tools/foxglove_comm.cpp`
- Risk: If the Foxglove SDK prebuilt binaries are incompatible with the target platform's ABI (e.g., aarch64 vs x86_64), this will only be discovered at runtime during competition.
- Priority: Low (Foxglove is a visualization tool, not critical to auto_aim function)

## Migration Risks (sp_vision_25 -> TGU_Robocore_2027)

### Dependency Gap Summary

| sp_vision_25 Dependency | TGU_Robocore_2027 Equivalent | Risk Level |
|---|---|---|
| Eigen3 (linear algebra) | Commented out in CMakeLists.txt | CRITICAL |
| fmt (formatting) | std::format (C++20) | Medium |
| spdlog (logging) | custom Logger with std::format | Medium |
| yaml-cpp (config) | toml++ (TOML format) | High |
| nlohmann_json | Not available | Low (unused) |
| OpenVINO (neural net) | Not available | Low (deferred) |
| OpenCV | OpenCV (same) | None |
| HikRobot MVS SDK | Aravis (open source) | High |
| MindVision SDK | Not available | High |

### Camera API Incompatibility (Critical for Migration)

The entire camera subsystem must be rewritten: sp_vision_25 uses proprietary HikRobot MVS SDK (`MvCameraControl`) and MindVision SDK (`MVSDK`), while Robocore uses Aravis (open-source GigE Vision/GenICam). Camera initialization, parameter setting, image acquisition, and buffer management follow completely different APIs. The current `test_camera.cpp` demonstrates a working Aravis flow, but the `io/hikrobot/` driver is empty.

### ROS2 Integration Gap

sp_vision_25 has optional ROS2 integration for sentry navigation communication (`sentry.cpp`, `publish2nav.cpp`, `subscribe2nav.cpp`). Robocore explicitly excludes ROS2. If sentry navigation integration is needed later, a custom IPC mechanism (shared memory, Unix sockets, or Foxglove-based) must be developed from scratch.

---

*Concerns audit: 2026-05-01*
