# Testing Patterns

**Analysis Date:** 2026-05-01

## Test Framework

**Runner:**
- No formal test framework (no Google Test, Catch2, CTest, or similar)
- Tests are standalone executables with `int main()` that exercise specific modules
- Manual pass/fail determined by visual inspection of console output or saved files

**Config:**
- Build configuration in `task/CMakeLists.txt`
- No test runner config files found

**Run Commands:**
```bash
mkdir build && cd build
cmake ..
make test_serial test_logger test_camera read_toml test_foxglove
./test_serial          # Serial communication test
./test_logger          # Logger test
./test_camera          # Camera capture test
./read_toml            # TOML config parsing test
```

**Notes:**
- `make test` / `ctest` is NOT configured -- no `enable_testing()` or `add_test()` in any `CMakeLists.txt`
- Tests must be run individually as executables

## Test File Organization

**Location:**
- All test files are in `task/test/`
- Tests are defined as executables in `task/CMakeLists.txt`

**Naming:**
- Test files: `test_<module>.cpp` -- e.g., `test_serial.cpp`, `test_logger.cpp`, `test_camera.cpp`
- Config/utility tests: `<action>_<module>.cpp` -- e.g., `read_toml.cpp`

**Structure:**
```
task/test/
├── test_serial.cpp      # Serial communication + CRC verification
├── test_logger.cpp      # Logger levels and formatting
├── test_camera.cpp      # Aravis camera capture + OpenCV save
├── test_foxglove.cpp    # Foxglove WebSocket (fully commented out)
└── read_toml.cpp        # TOML config parsing and matrix reading
```

## Test Structure

**Suite Organization:**
No test suites exist. Each file is a single `main()` function that tests one module end-to-end.

**Pattern across tests:**
```cpp
// 1. Initialize dependencies (Logger, config, etc.)
// 2. Instantiate the module under test
// 3. Call its methods
// 4. Use std::cout or Logger for output
// 5. Return 0 on success, -1 on failure
```

**Specific example from `test_serial.cpp`:**
```cpp
int main() {
    // Initialize logger
    tools::Logger::instance().init(cfg);

    // Create module under test
    io::Serial serial;
    serial.open("/dev/ttyACM0", 2000000);

    // Register callback
    serial.recv<RecvPackage>([](const auto &pkt) {
        // Validate CRC
        if (!tools::check_crc16(...)) {
            LOG_WARN(MODULE, "CRC check failed");
            return;
        }
    });

    // Main loop
    while (true) {
        serial.spin_once();
        serial.send(sendpkg);
    }
}
```

**None of the test files use assertions.** Tests rely on console output and manual inspection of:
- Printed values (std::cout)
- Log output (Logger / std::cerr)
- Saved files (e.g., `test_camera.cpp` saves `test_bgr.png`)

## Mocking

**Framework:** None detected. No mocking library is used.

**Patterns:**
- No mocking is used. Tests rely on real hardware devices:
  - `test_serial.cpp` requires a real serial device (`/dev/ttyACM0`)
  - `test_camera.cpp` requires a real Aravis-compatible camera
- Hardware-dependent tests cannot run in CI or offline environments

**What to Mock:**
- Serial port (for protocol/logic tests without hardware)
- Camera (for image processing tests with pre-recorded data)

**What NOT to Mock:**
- CRC functions (they are pure computation, easily testable directly)

## Fixtures and Factories

**Test Data:**
- `testconfig.toml` in `/home/eldwen/TGU_Robocore_2027/config/` provides configuration values used by `read_toml.cpp`
- Camera intrinsics and distortion coefficients read from TOML arrays in `testconfig.toml`

**Example TOML config** (`config/testconfig.toml`):
```toml
title = "testconfig"

[game]
enemy_color = "blue"

[camera]
camera_name = "hikrobot"
exposure_ms = 5
gain = 10
```

**Location:**
- Configuration: `config/testconfig.toml`
- No dedicated test fixture directory or factory functions exist

## Coverage

**Requirements:** None enforced. No coverage tooling detected.

**Current coverage gaps:**
- `io/serial/` -- tested via `test_serial.cpp` (only positive path, no error-path testing)
- `tools/logger.cpp` -- tested via `test_logger.cpp` (verifies log output format)
- `tools/crc.cpp` -- NOT directly tested (used transitively by `test_serial.cpp`)
- `io/hikrobot/` -- NOT tested (stub files only)
- `tools/foxglove_comm.cpp` -- NOT tested (`test_foxglove.cpp` is fully commented out)
- `app/auto_aim/` -- NOT tested (stub files only)

## Test Types

**Unit Tests:**
- None. No isolated unit tests exist for individual classes or functions.

**Integration Tests:**
- `test_serial.cpp` -- hardware integration: tests serial I/O with CRC validation end-to-end
- `test_camera.cpp` -- hardware integration: tests Aravis camera discovery, capture, and OpenCV conversion
- `test_logger.cpp` -- module integration: tests Logger initialization and all log levels
- `read_toml.cpp` -- module integration: tests TOML file parsing and array extraction

**E2E Tests:**
- Not used. No full-system or simulated competition tests exist.

**State of tests:**
- `test_foxglove.cpp` is entirely commented out, indicating the Foxglove integration is not testable yet
- `sentry.cpp` (main entry point) is an empty placeholder with only a file header comment

## Common Patterns

**Logger initialization pattern** (used in every test):
```cpp
tools::LoggerConfig cfg{
    .level = tools::LogLevel::Debug,
    .enable_console = true,
    .enable_file = false,
};
tools::Logger::instance().init(cfg);
```

**Serial test pattern:**
```cpp
io::Serial serial;
serial.open("/dev/ttyACM0", 2000000);
serial.recv<RecvPackage>([](const auto &pkt) {
    if (!tools::check_crc16(...)) {
        LOG_WARN(MODULE, "CRC check failed");
        return;
    }
});
while (true) {
    serial.spin_once();
    sendpkg.data++;
    serial.send(sendpkg);
}
```

**Camera test pattern** (Aravis API, procedural):
```cpp
arv_update_device_list();
const char* id = arv_get_device_id(0);
ArvCamera* camera = arv_camera_new(id, &error);
arv_camera_set_exposure_time(camera, 100000.0, &error);
ArvStream* stream = arv_camera_create_stream(camera, nullptr, nullptr, &error);
// ... capture loop ...
```

## Comparison with Source Project (sp_vision_25)

**Key differences from the project being migrated FROM (`/home/eldwen/sp_vision_25`):**

| Aspect | TGU_Robocore_2027 | sp_vision_25 |
|--------|-------------------|--------------|
| Test location | `task/test/` | `tests/` (project root) |
| Test file naming | `test_<module>.cpp` | `<module>_test.cpp` |
| CMake target naming | `test_serial` | `auto_aim_test` |
| Test framework | None (bare `main()`) | None (bare `main()`) |
| Number of tests | 4 active, 1 commented | ~17 active |
| Hardware dependency | Serial, Camera | CBoard, Gimbal, Camera |
| CLI argument parsing | None | `cv::CommandLineParser` |
| Visualization | `cv::imwrite` only | `cv::imshow`, `tools::Plotter` |
| `Exiter` pattern | Not used | Common (`while (!exiter.exit())`) |

**sp_vision_25 test patterns of note:**
- Uses `cv::CommandLineParser` for configurable test paths (e.g., `auto_aim_test.cpp`)
- Uses `tools::Exiter` for graceful shutdown on Ctrl+C (e.g., `cboard_test.cpp`)
- Uses `tools::Plotter` for real-time data visualization during tests
- Tests instantiate full processing pipelines (YOLO + Solver + Tracker + Aimer in `auto_aim_test.cpp`)
- `minimum_vision_system.cpp` serves as a full system integration test with multi-threading (detector thread + main loop)

**Improvements to consider from sp_vision_25:**
1. Add CLI argument parsing to tests for configurable paths
2. Use `Exiter` pattern for clean shutdown instead of infinite `while(true)`
3. Add a multi-threading test pattern for camera + processing pipelines
4. Standardize on either `test_<module>.cpp` or `<module>_test.cpp` naming (currently different between the two projects)

---

*Testing analysis: 2026-05-01*
