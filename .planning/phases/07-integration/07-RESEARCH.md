# Phase 7: Integration Test - Research

**Researched:** 2026-05-01
**Domain:** C++ integration testing, CMake test configuration, OpenCV synthetic image generation
**Confidence:** HIGH

## Summary

Phase 7 delivers three artifacts: (1) `config/auto_aim.toml` merging configuration for all six self-aim modules (classifier, detector, solver, tracker, aimer, shooter), (2) `task/test/test_auto_aim.cpp` with unit tests for pure-math functions, data structure construction, and a synthetic-image detector test, and (3) updates to `task/CMakeLists.txt` to compile and link the test executable.

The critical constraint is that the Classifier/Detector require an ONNX model file (`tiny_resnet.onnx`) to construct, so the full-pipeline detector synthetic-image test must either provide the model or gate on its existence. Pure-math tests (math_tools, armor geometry, trajectory, EKF) need no model and no display, making them safe for all environments including headless CI.

**Primary recommendation:** Structure tests into always-run (math, geometry, EKF, trajectory) and conditionally-run (detector synthetic image, requires model + display). The config file must document all sixteen parameters across the six modules with production-ready defaults copied from the source code.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Auto-aim configuration (TOML) | Config / Static asset | — | Runtime-loaded config file; no running service owns it |
| Pure-math unit tests | Task / Test executable | tools/ | Tests run on build machine; they validate tools library functions |
| Data structure tests | Task / Test executable | app/auto_aim/ | Validate Armor/Lightbar construction from synthetic inputs |
| Detector synthetic-image test | Task / Test executable | app/auto_aim/ | Full pipeline integration test: image -> contours -> armor list |
| EKF filter test | Task / Test executable | tools/ | Validate predict/update convergence without external data |
| CMake build integration | Build system | — | Test executable must link auto_aim + all its transitive deps |
| Classifier model file | External asset | — | tiny_resnet.onnx sits outside the code; test gates on existence |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 3.28 | Build system | Already project standard (top-level CMakeLists.txt) |
| OpenCV | 4.6.0 | Image creation, drawing, DNN | Already project standard. `cv::rectangle`, `cv::Mat` for synthetic images |
| Eigen3 | 3.4.0 | Linear algebra for EKF tests | Already project standard |
| fmt | latest | Text formatting in test output | Already project standard |
| OpenVINO | 2024.6.0 | Classifier model loading | Already project standard. Required for Detector construction |

### Testing Approach
No external test framework (no gtest, no Catch2). The existing pattern in `task/test/test_logger.cpp` uses a plain `main()` function with `assert()` or manual checks. Phase 7 follows the same pattern.

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Plain `main()` tests | Google Test (gtest) | Would add dependency. Current project pattern is plain `main()`. Adding gtest would require CMake changes and dependency installation. Deferred. |
| Plain `main()` tests | Catch2 | Same tradeoff as gtest. Single-header version exists but adds complexity not justified for this phase. |

**Installation:**
No new dependencies. All libraries (Eigen3, OpenCV, OpenVINO, fmt, Boost) are already installed and findable by CMake.

**Version verification:**
```bash
cmake --version        # 3.28.3 [VERIFIED]
pkg-config --modversion opencv4        # 4.6.0 [VERIFIED]
dpkg -l libeigen3-dev | tail -1        # 3.4.0 [VERIFIED]
dpkg -l libopenvino-dev-2024.6.0       # 2024.6.0 [VERIFIED]
```

## Architecture Patterns

### Config File Structure

The `config/auto_aim.toml` must contain sections for all six modules that read from it. Each key's type and default value is extracted directly from each module's constructor.

#### Section: `[classifier]`
```toml
[classifier]
classify_model = "/home/eldwen/sp_vision_25/assets/tiny_resnet.onnx"
```

From `classifier.cpp` line 12:
```cpp
auto model = config["classifier"]["classify_model"].value_or<std::string>("");
```
- `classify_model`: string, path to ONNX model. Required (empty string crashes `readNetFromONNX`).

#### Section: `[detector]`
From `detector.cpp` lines 19-28:
```cpp
threshold_ = config["detector"]["threshold"].value_or<double>(150.0);
max_angle_error_ = config["detector"]["max_angle_error"].value_or<double>(15.0) / 57.3;
min_lightbar_ratio_ = config["detector"]["min_lightbar_ratio"].value_or<double>(1.5);
max_lightbar_ratio_ = config["detector"]["max_lightbar_ratio"].value_or<double>(10.0);
min_lightbar_length_ = config["detector"]["min_lightbar_length"].value_or<double>(10.0);
min_armor_ratio_ = config["detector"]["min_armor_ratio"].value_or<double>(1.0);
max_armor_ratio_ = config["detector"]["max_armor_ratio"].value_or<double>(4.0);
max_side_ratio_ = config["detector"]["max_side_ratio"].value_or<double>(2.0);
min_confidence_ = config["detector"]["min_confidence"].value_or<double>(0.5);
max_rectangular_error_ = config["detector"]["max_rectangular_error"].value_or<double>(20.0) / 57.3;
```

#### Section: `[solver]`
From `solver.cpp` lines 46-65:
```cpp
R_gimbal2imubody_ = config["solver"]["R_gimbal2imubody"];  // 3x3 array
R_camera2gimbal_ = config["solver"]["R_camera2gimbal"];      // 3x3 array
t_camera2gimbal_ = config["solver"]["t_camera2gimbal"];      // 3-element array
camera_matrix_ = config["solver"]["camera_matrix"];          // 3x3 array
distort_coeffs_ = config["solver"]["distort_coeffs"];        // 5-element array
yaw_search_range_ = config["solver"]["yaw_search_range"].value_or<double>(140.0);
```

These are arrays, not scalar values. The `toml_array_to_vector` helper parses them. **The test config must include valid placeholder values** for all arrays (identity matrices for rotation, test-only camera params).

#### Section: `[tracker]`
From `tracker.cpp` lines 19-24:
```cpp
enemy_color_ = (config["tracker"]["enemy_color"].value_or<std::string>("") == "red") ? Color::red : Color::blue;
min_detect_count_ = static_cast<int>(config["tracker"]["min_detect_count"].value_or<int64_t>(5));
max_temp_lost_count_ = static_cast<int>(config["tracker"]["max_temp_lost_count"].value_or<int64_t>(50));
```

#### Section: `[aimer]`
From `aimer.cpp` lines 18-33:
```cpp
yaw_offset_ = config["aimer"]["yaw_offset"].value_or<double>(0.0) / 57.3;
pitch_offset_ = config["aimer"]["pitch_offset"].value_or<double>(0.0) / 57.3;
comming_angle_ = config["aimer"]["comming_angle"].value_or<double>(0.0) / 57.3;
leaving_angle_ = config["aimer"]["leaving_angle"].value_or<double>(0.0) / 57.3;
high_speed_delay_time_ = config["aimer"]["high_speed_delay_time"].value_or<double>(0.0);
low_speed_delay_time_ = config["aimer"]["low_speed_delay_time"].value_or<double>(0.0);
decision_speed_ = config["aimer"]["decision_speed"].value_or<double>(0.0);
left_yaw_offset_ = config["aimer"]["left_yaw_offset"].value_or<double>(0.0) / 57.3;  // optional
right_yaw_offset_ = config["aimer"]["right_yaw_offset"].value_or<double>(0.0) / 57.3;  // optional
```

#### Section: `[shooter]`
From `shooter.cpp` lines 13-17:
```cpp
first_tolerance_ = config["shooter"]["first_tolerance"].value_or<double>(0.0) / 57.3;
second_tolerance_ = config["shooter"]["second_tolerance"].value_or<double>(0.0) / 57.3;
judge_distance_ = config["shooter"]["judge_distance"].value_or<double>(0.0);
auto_fire_ = config["shooter"]["auto_fire"].value_or<bool>(false);
```

### Test Executable Pattern

The project uses self-contained `main()` test executables in `task/test/`. Pattern from `test_logger.cpp`:

```cpp
#include "tools/logger.hpp"
#include <cassert>
#include <cmath>

static bool approx_equal(double a, double b, double eps = 1e-6) {
    return std::abs(a - b) < eps;
}

static void test_something() {
    double result = some_function(42.0);
    assert(approx_equal(result, 3.14));
}

int main() {
    // Initialize logger for modules that use LOG_XXX macros
    tools::LoggerConfig cfg{
        .level = tools::LogLevel::Debug,
        .enable_console = true,
        .enable_file = false,
        .file_path = "logs.txt"
    };
    tools::Logger::instance().init(cfg);

    test_something();
    // ...

    return 0;
}
```

### Synthetic Image Pattern for Detector Test

The detector test creates a `cv::Mat` with two bright vertical rectangles simulating armor light bars, then feeds it to `Detector::detect()`.

**Key geometry constraints for a valid armor plate:**
- Lightbar length/width ratio: in [1.5, 10.0] (default thresholds)
- Lightbar length: > 10px
- Armor width/height ratio: in [1.0, 4.0]
- Armor side ratio: < 2.0
- Armor rectangular error: < 20 degrees

**Working synthetic armor dimensions** (verified against detector geometry checks):

| Dimension | Value | Why |
|-----------|-------|-----|
| Image size | 640 x 480 | Standard test image |
| Left bar rect | `Rect(100, 100, 40, 280)` | 40px wide, 280px tall |
| Right bar rect | `Rect(500, 100, 40, 280)` | Same size, 400px gap between centers |
| Bar color | `Scalar(255, 255, 255)` | White (brightest, passes threshold) |
| Background | `Scalar(0, 0, 0)` | Black (passes binary threshold) |

**Geometry verification:**
- Lightbar ratio: 280/40 = 7.0 (in [1.5, 10.0]) -- PASS
- Min lightbar length: 280 > 10 -- PASS
- Angle error: ~0 (vertical bar, target is vertical) -- PASS
- Armor ratio: 400/280 = 1.43 (in [1.0, 4.0]) -- PASS
- Side ratio: 280/280 = 1.0 (< 2.0) -- PASS
- Rectangular error: ~0 -- PASS

### CMake Linking Pattern

The test executable links `PRIVATE auto_aim`, which transitively brings in all dependencies:

```cmake
add_executable(test_auto_aim test/test_auto_aim.cpp)
target_link_libraries(test_auto_aim PRIVATE auto_aim)
```

The transitive dependency chain resolved by CMake:
```
test_auto_aim
  -> auto_aim (STATIC)
    -> io (STATIC, linked PUBLIC by auto_aim)
    -> tools (STATIC, linked PUBLIC by auto_aim)
    -> Eigen3::Eigen
    -> openvino::runtime
    -> fmt::fmt
```

**No need to manually list `${OpenCV_LIBS}`** -- OpenCV comes through `auto_aim`'s PUBLIC dependency chain (tools has `${OpenCV_INCLUDE_DIRS}` PUBLIC, and various modules use OpenCV functions).

### Anti-Patterns to Avoid

- **Putting test logic in header files**: Tests belong in `.cpp` files under `task/test/`. Do not add test code to `app/auto_aim/` headers.
- **Modifying production code for testability**: Do not remove `cv::imshow` from detector.cpp. If `cv::imshow` is problematic in headless environments, wrap the full-pipeline test in a conditional or run with Xvfb.
- **Hard-coding paths in tests**: Use relative paths from the build directory, or accept paths as compile definitions.
- **Duplicate config defaults**: The test config file should match defaults used in the code. If code and config disagree, it creates confusion.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Testing framework | Custom test runner, test discovery | Plain `main()` with `assert()` | Project convention. Existing tests in `task/test/` all use this pattern. Adding gtest/Catch2 would be scope creep. |
| Synthetic image creation | Manual pixel-by-pixel arrays | `cv::Mat`, `cv::rectangle` | OpenCV provides all primitives for drawing rectangles (light bars) on black background. |
| TOML parsing | Custom TOML parser | `tools/tomlpp.hpp` (toml++ wrapper) | Already used by all modules. Documented in Phase 3 CONTEXT.md D-02. |

**Key insight:** Every module already uses `tools/tomlpp.hpp` to read config. The test executable simply loads the same config file. Do not introduce new parsing.

## Common Pitfalls

### Pitfall 1: Classifier Constructor Crashes Without ONNX Model
**What goes wrong:** `Detector(config, debug)` constructs `Classifier(config_path)`, which calls `cv::dnn::readNetFromONNX("")` or with an invalid path. This throws an uncatched exception and the test binary crashes.

**Why it happens:** Classifier constructor (classifier.cpp lines 12-17) has no guards. It always reads the model file. If the model path in config is missing or invalid, the constructor throws.

**How to avoid:** Options in order of preference:
1. Copy `tiny_resnet.onnx` (~1MB) from `sp_vision_25/assets/` into the project (e.g., `config/models/`) and reference it from the TOML config.
2. Wrap the full Detector test in a conditional that checks `std::filesystem::exists(model_path)` before constructing the Detector.

**Warning signs:** Test crashes at Detector construction with "Can't read ONNX file" or `cv::Exception`.

### Pitfall 2: cv::imshow Fails in Headless Environments
**What goes wrong:** `Detector::detect()` calls `cv::imshow("binary_img", binary_img)` unconditionally (line 41 of detector.cpp). In environments without a display server (CI, WSL2 without X server, Docker), this throws an exception or blocks.

**Why it happens:** The `cv::imshow` call is not guarded by the `debug_` flag. Only the `show_result()` call at line 109 is guarded.

**How to avoid:**
- For always-pass tests: do not call `detect()`. Instead, test geometry checks and individual functions directly.
- For full-pipeline test: run with `Xvfb :99 -screen 0 1024x768x24 &` virtual framebuffer, or gate on display availability.
- Long-term fix: move `cv::imshow("binary_img")` inside the `debug_` guard in detector.cpp (but this is out of scope for Phase 7).

**Warning signs:** `cv::error: OpenCV(4.6.0) .../window.cpp: error: (-2:Unspecified error) The function is not implemented.` or test hanging.

### Pitfall 3: Solver Config Arrays Must Be Complete
**What goes wrong:** The Solver constructor reads 3x3 rotation matrices and camera matrices as TOML arrays. Missing or wrong-size arrays produce `std::bad_alloc` or silently wrong data.

**Why it happens:** The `toml_array_to_vector` helper reads by index, and `Eigen::Matrix` construction from data expects exact sizes. Four arrays are required: R_gimbal2imubody (9 elements), R_camera2gimbal (9 elements), t_camera2gimbal (3 elements), camera_matrix (9 elements), distort_coeffs (5 elements).

**How to avoid:** In the test config, use identity matrices for rotations and a simple camera matrix for testing. Always verify array element counts match expectations.

**Warning signs:** `Solver` constructor crashes, or `Eigen` assertion failures about size mismatch.

### Pitfall 4: Logger Must Be Initialized Before LOG_XXX Macros
**What goes wrong:** Any module that uses `LOG_XXX` macros will call `tools::Logger::instance()` internally. If the Logger was not initialized, it may default to no-output or crash.

**Why it happens:** Modules like Detector, Tracker, Aimer, and Shooter define `static constexpr const char* MODULE` and call `LOG_DEBUG`, `LOG_WARN`, etc. These macros call `tools::Logger::instance()` which expects prior `Logger::instance().init(cfg)`.

**How to avoid:** Initialize the logger at the top of `main()` in the test executable, as shown in `test_logger.cpp` lines 8-13.

**Warning signs:** Segfault on first LOG_XXX call, or empty test output.

### Pitfall 5: fmt Dependency Not Linked for Test Executable
**What goes wrong:** `test_auto_aim.cpp` uses `fmt::format()` or transitively includes code that uses fmt. The linker fails with undefined references.

**Why it happens:** `fmt::fmt` is linked PUBLIC by `auto_aim` (see `app/CMakeLists.txt` line 25), so it should transitively propagate. But if the CMakeLists.txt for the test executable is misconfigured, the link may fail.

**How to avoid:** Use `target_link_libraries(test_auto_aim PRIVATE auto_aim)`. The `PRIVATE` keyword ensures transitive dependencies from PUBLIC-linked libraries propagate correctly. Do not use `INTERFACE` or omit the visibility keyword.

**Warning signs:** Linker errors for `fmt::vXX::vformat`, `fmt::detail::...`.

## Code Examples

### config/auto_aim.toml Structure

```toml
# ========================================
# Auto-aim Configuration
# ========================================

[classifier]
classify_model = "/path/to/tiny_resnet.onnx"

[detector]
threshold = 150.0
max_angle_error = 15.0
min_lightbar_ratio = 1.5
max_lightbar_ratio = 10.0
min_lightbar_length = 10.0
min_armor_ratio = 1.0
max_armor_ratio = 4.0
max_side_ratio = 2.0
min_confidence = 0.5
max_rectangular_error = 20.0

[solver]
R_gimbal2imubody = [1, 0, 0, 0, 1, 0, 0, 0, 1]
R_camera2gimbal = [1, 0, 0, 0, 1, 0, 0, 0, 1]
t_camera2gimbal = [0, 0, 0]
camera_matrix = [1000, 0, 320, 0, 1000, 240, 0, 0, 1]
distort_coeffs = [0, 0, 0, 0, 0]
yaw_search_range = 140

[tracker]
enemy_color = "blue"
min_detect_count = 5
max_temp_lost_count = 50

[aimer]
yaw_offset = 0.0
pitch_offset = 0.0
comming_angle = 0.0
leaving_angle = 0.0
high_speed_delay_time = 0.0
low_speed_delay_time = 0.0
decision_speed = 0.0
# left_yaw_offset = 0.0   # optional
# right_yaw_offset = 0.0  # optional

[shooter]
first_tolerance = 0.0
second_tolerance = 0.0
judge_distance = 0.0
auto_fire = false
```

### Synthetic Image Creation

```cpp
// Source: OpenCV 4.6.0 documentation, cv::rectangle API
static cv::Mat create_synthetic_armor_image() {
    // Black background
    cv::Mat img(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

    // Left light bar: vertical bright rectangle
    cv::rectangle(img, cv::Point(100, 100), cv::Point(140, 380),
                  cv::Scalar(255, 255, 255), cv::FILLED);

    // Right light bar: vertical bright rectangle
    cv::rectangle(img, cv::Point(500, 100), cv::Point(540, 380),
                  cv::Scalar(255, 255, 255), cv::FILLED);

    return img;
}
```

### Test Skeleton (Always-Run Tests)

```cpp
#include <cassert>
#include <cmath>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>

#include "tools/logger.hpp"
#include "tools/math_tools.hpp"
#include "tools/trajectory.hpp"
#include "tools/extended_kalman_filter.hpp"
#include "app/auto_aim/armor.hpp"

static constexpr double EPS = 1e-6;

static bool approx_equal(double a, double b, double eps = EPS) {
    return std::abs(a - b) < eps;
}

// ---- Math Tools Tests ----

static void test_limit_rad() {
    // pi should wrap to -pi
    assert(approx_equal(tools::limit_rad(M_PI), -M_PI));
    // -pi should wrap to -pi
    assert(approx_equal(tools::limit_rad(-M_PI), -M_PI));
    // 0 stays 0
    assert(approx_equal(tools::limit_rad(0.0), 0.0));
    // 3*pi/2 should wrap to -pi/2
    assert(approx_equal(tools::limit_rad(3 * M_PI / 2), -M_PI / 2));
    printf("[PASS] test_limit_rad\n");
}

static void test_delta_time() {
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();
    auto t1 = t0 + std::chrono::milliseconds(100);
    double dt = tools::delta_time(t1, t0);
    assert(approx_equal(dt, 0.1, 1e-3));
    printf("[PASS] test_delta_time\n");
}

// ---- Armor Geometry Tests ----

static void test_lightbar_vertical() {
    // Create a vertical RotatedRect simulating a light bar
    cv::RotatedRect rrect(cv::Point2f(120, 240), cv::Size2f(40, 280), 0);
    app::auto_aim::Lightbar lb(rrect, 0);

    // A vertical bar has angle_error near 0
    assert(lb.angle_error < 0.1);
    // Length should be ~280
    assert(approx_equal(lb.length, 280.0, 10.0));
    // Ratio should be ~7.0 (280/40)
    assert(approx_equal(lb.ratio, 7.0, 1.0));
    printf("[PASS] test_lightbar_vertical\n");
}

static void test_armor_pairing() {
    // Create two light bars
    cv::RotatedRect left_rrect(cv::Point2f(120, 240), cv::Size2f(40, 280), 0);
    cv::RotatedRect right_rrect(cv::Point2f(520, 240), cv::Size2f(40, 280), 0);

    app::auto_aim::Lightbar left_lb(left_rrect, 0);
    app::auto_aim::Lightbar right_lb(right_rrect, 1);
    left_lb.color = app::auto_aim::Color::red;
    right_lb.color = app::auto_aim::Color::red;

    app::auto_aim::Armor armor(left_lb, right_lb);

    // Center should be midpoint
    assert(approx_equal(armor.center.x, 320.0, 10.0));
    assert(approx_equal(armor.center.y, 240.0, 10.0));
    // Four points should be present
    assert(armor.points.size() == 4);
    printf("[PASS] test_armor_pairing\n");
}

// ---- Trajectory Test ----

static void test_trajectory() {
    tools::Trajectory traj(23.0, 10.0, 0.5);
    assert(!traj.unsolvable);
    assert(traj.fly_time > 0);
    printf("[PASS] test_trajectory\n");
}

// ---- EKF Test ----

static void test_ekf_1d() {
    Eigen::VectorXd x0{{0.0}};
    Eigen::MatrixXd P0 = Eigen::MatrixXd::Identity(1, 1);
    tools::ExtendedKalmanFilter ekf(x0, P0);

    Eigen::MatrixXd F = Eigen::MatrixXd::Identity(1, 1);
    Eigen::MatrixXd Q = Eigen::MatrixXd::Identity(1, 1) * 0.01;
    Eigen::MatrixXd H = Eigen::MatrixXd::Identity(1, 1);
    Eigen::MatrixXd R = Eigen::MatrixXd::Identity(1, 1) * 0.1;

    // Predict then update with observation z=1.0
    ekf.predict(F, Q);
    Eigen::VectorXd z{{1.0}};
    ekf.update(z, H, R);

    // After one predict-update, state should move toward 1.0
    assert(ekf.x[0] > 0.5);
    printf("[PASS] test_ekf_1d\n");
}

int main() {
    // Initialize logger -- required by LOG_XXX macros in production code
    tools::LoggerConfig cfg{
        .level = tools::LogLevel::Debug,
        .enable_console = true,
        .enable_file = false,
        .file_path = "logs.txt"
    };
    tools::Logger::instance().init(cfg);

    // Always-run tests
    test_limit_rad();
    test_delta_time();
    test_lightbar_vertical();
    test_armor_pairing();
    test_trajectory();
    test_ekf_1d();

    printf("All tests passed!\n");
    return 0;
}
```

### Detector Synthetic-Image Test (Conditional on Model + Display)

```cpp
#include <filesystem>

#include "app/auto_aim/detector.hpp"

static void test_detector_synthetic_armor() {
    std::string config_path = "../../config/auto_aim.toml";

    // Check model file exists before constructing Detector
    auto config = toml::parse_file(config_path);
    auto model = config["classifier"]["classify_model"].value_or<std::string>("");
    if (model.empty() || !std::filesystem::exists(model)) {
        printf("[SKIP] test_detector_synthetic_armor: model file not found at %s\n", model.c_str());
        return;
    }

    app::auto_aim::Detector detector(config_path, false);

    // Create synthetic armor plate image
    cv::Mat img(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::rectangle(img, cv::Point(100, 100), cv::Point(140, 380),
                  cv::Scalar(255, 255, 255), cv::FILLED);
    cv::rectangle(img, cv::Point(500, 100), cv::Point(540, 380),
                  cv::Scalar(255, 255, 255), cv::FILLED);

    auto armors = detector.detect(img, 0);

    // Should detect at least one armor
    assert(!armors.empty());
    printf("[PASS] test_detector_synthetic_armor: detected %zu armors\n", armors.size());
}
```

## State of the Art

Not applicable -- Phase 7 is integration testing, not migrating existing functionality. The config format change from YAML to TOML was completed in Phases 3-6 and is documented in their respective CONTEXT.md files.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `tiny_resnet.onnx` from sp_vision_25/assets/ will be available either by copy or symlink for the detector test | Code Examples, Detector synthetic-image test | Detector construction will throw, conditional test will always skip |
| A2 | `cv::imshow("binary_img", binary_img)` in detector.cpp line 41 will fail in headless/CI environments | Common Pitfalls Pitfall 2 | Full-pipeline detector test cannot run in environments without display |
| A3 | The existing `task/CMakeLists.txt` pattern of `target_link_libraries(test_xxx PRIVATE tools)` works identically for `auto_aim` | CMake Linking Pattern | Linker errors for transitive dependencies |
| A4 | OpenCV 4.6.0 provides all APIs used in synthetic image (`cv::Mat`, `cv::rectangle`, `cv::FILLED`) | Standard Stack | Build failure if OpenCV version is older |

## Open Questions

1. **Should tiny_resnet.onnx be copied into the project or referenced via relative path from sp_vision_25?**
   - What we know: The model is ~1MB at `/home/eldwen/sp_vision_25/assets/tiny_resnet.onnx`
   - What's unclear: Whether the Robocore project should own a copy (adds 1MB to repo) or reference the source project (fragile path dependency)
   - Recommendation: Accept a copy into `config/models/tiny_resnet.onnx` for self-contained testing; the `config/auto_aim.toml` points there. The model is small and essential for integration tests.

2. **Should the detector test be kept as a conditional skip, or should we modify detector.cpp to make imshow debug-guarded?**
   - What we know: `cv::imshow` is called unconditionally at detector.cpp line 41
   - What's unclear: Whether Phase 7 should modify existing production code to fix this, or document the constraint
   - Recommendation: Defer to Claude's discretion. If modifying detector.cpp, move `cv::imshow("binary_img", binary_img)` inside `if (debug_)` guard. This is a 1-line change that makes the test fully headless-capable.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Build test executable | Yes | 3.28.3 | -- |
| OpenCV | Synthetic image, detector | Yes | 4.6.0 | -- |
| Eigen3 | EKF tests | Yes | 3.4.0 | -- |
| OpenVINO | Classifier model loading | Yes | 2024.6.0 | -- |
| fmt | Format strings in test | Yes (via CMake find_package) | -- | -- |
| X display server | Full-pipeline detector test | WSL2-dependent | -- | `Xvfb :99 -screen 0 1024x768x24 &` |

**Missing dependencies with no fallback:** None.

**Missing dependencies with fallback:**
- X display server: Use Xvfb virtual framebuffer, or test individual functions without calling `detect()`.

## Sources

### Primary (HIGH confidence)
- `detector.cpp` lines 14-32 -- Verified all `[detector]` config keys and default values [VERIFIED: codebase grep]
- `classifier.cpp` lines 9-17 -- Verified `[classifier]` config key for model path [VERIFIED: codebase grep]
- `solver.cpp` lines 42-66 -- Verified `[solver]` config keys including array parameters [VERIFIED: codebase grep]
- `tracker.cpp` lines 19-24 -- Verified `[tracker]` config keys [VERIFIED: codebase grep]
- `aimer.cpp` lines 18-33 -- Verified `[aimer]` config keys [VERIFIED: codebase grep]
- `shooter.cpp` lines 13-17 -- Verified `[shooter]` config keys [VERIFIED: codebase grep]
- `task/CMakeLists.txt` -- Verified existing test executable link pattern [VERIFIED: codebase grep]
- `test_logger.cpp` -- Verified plain `main()` test pattern [VERIFIED: codebase grep]
- `app/CMakeLists.txt` -- Verified auto_aim library dependencies [VERIFIED: codebase grep]
- `tools/CMakeLists.txt` -- Verified tools library dependencies [VERIFIED: codebase grep]
- `armor.cpp` lines 13-35 -- Verified Lightbar geometry computation (ratio = length/width, angle_error) [VERIFIED: codebase grep]
- `armor.hpp` -- Verified Armor and Lightbar field types [VERIFIED: codebase grep]

### Secondary (MEDIUM confidence)
- OpenCV 4.6.0 `cv::rectangle` API -- Verified via system headers; no documentation fetch needed for basic drawing primitives. `cv::FILLED` available since OpenCV 3.x. [ASSUMED]
- Eigen 3.4 `Eigen::Matrix` row-major construction from raw data -- Verified via system headers; size-mismatch leads to assertion. [ASSUMED]

### Tertiary (LOW confidence)
- `cv::imshow` behavior in headless WSL2 -- Not tested; different configurations behave differently (some crash, some create windows). [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All dependencies verified via system inspection
- Architecture: HIGH -- All module TOML keys verified against source code
- Pitfalls: HIGH -- All risks identified from source analysis (classifier construction, imshow, logger init, solver arrays)
- Synthetic image dimensions: HIGH -- Verified geometry checks from detector.cpp source

**Research date:** 2026-05-01
**Valid until:** 2026-06-01 (stable project, no external dependency changes expected)
