# Phase 3: 灯条检测与装甲板匹配 - Research

**Researched:** 2026-05-01
**Domain:** Computer vision: lightbar detection, armor matching, ONNX/OpenVINO classification
**Confidence:** MEDIUM

## Summary

This phase migrates two core vision modules from `sp_vision_25` to Robocore: **Detector** (lightbar extraction, armor pair matching, geometric validation, deduplication) and **Classifier** (OpenVINO-based digit classification on armor ROIs). Both modules are self-contained algorithmic code with no external runtime services.

The main migration surface is three-fold: (1) YAML-to-TOML config parsing using `tools/tomlpp.hpp`, (2) replacing `tools::logger()->debug(...)` with `LOG_DEBUG("DETECTOR", ...)` macros, and (3) adapting the source `auto_aim::` namespace to Robocore's `app::auto_aim::`. The classifier also adds an OpenVINO build dependency (`find_package(OpenVINO)`, `openvino::runtime`).

**Primary recommendation:** Create `app/auto_aim/detector.hpp/.cpp` and `app/auto_aim/classifier.hpp/.cpp` by porting the source files with TOML config, LOG_XXX macro, and namespace changes. Add OpenVINO and fmt build dependencies.

## User Constraints (from CONTEXT.md)

<user_constraints>
### Locked Decisions

- **D-01:** YAML 配置改为 TOML 格式
- **D-02:** 使用 Robocore 自带的 `tools/tomlpp.hpp` 读取
- **D-03:** 不再依赖 yaml-cpp
- **D-04:** 保留 fmt 库，不改为 std::format
- **D-05:** `fmt::format()` 调用保持不变
- **D-06:** 在 CMake 中添加 fmt 依赖
- **D-07:** 完整迁移 Classifier，包含 OpenVINO 数字识别
- **D-08:** 安装 OpenVINO（apt 安装）
- **D-09:** `classify()` 和 `ovclassify()` 均迁移
- **D-10:** 保留所有 cv::imshow 调用
- **D-11:** 通过 Detector 的 `debug_` 成员控制显示开关
- **D-12:** `tools::logger()->debug(...)` → `LOG_DEBUG("DETECTOR", ...)`
- **D-13:** 每个 .cpp 定义 `static constexpr const char* MODULE`
- **D-14:** 保留低置信度装甲板存图功能
- **D-15:** 保存路径由 debug 模式控制
- **D-16:** 不迁移 lightbar_points_corrector（原项目已禁用）
- **D-17:** 理由：作者 commit 583d50a 显示已切换回传统方法，原代码是死代码
- **D-18:** 迁移 `detect(Armor&, const cv::Mat&)` 函数
- **D-19:** 保留 YOLO ROI 精修逻辑，后续接 YOLO 可直接使用

### Claude's Discretion
- Detector 内部的算法逻辑和阈值保持不变

### Deferred Ideas (OUT OF SCOPE)
None
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DET-01 | 图像预处理（灰度化、二值化） | Uses `cv::cvtColor` + `cv::threshold` -- standard OpenCV, no special deps |
| DET-02 | 轮廓提取和灯条拟合（minAreaRect） | Uses `cv::findContours` + `cv::minAreaRect` -- standard OpenCV |
| DET-03 | 灯条几何校验（角度、长宽比、长度） | `Detector::check_geometry(Lightbar)` -- config thresholds, no new deps |
| DET-04 | 灯条颜色判定（RB 通道比较） | `Detector::get_color()` -- channel sum comparison, no new deps |
| DET-05 | 装甲板匹配（灯条配对） | Double loop over sorted lightbars, construct Armor from pair |
| DET-06 | 装甲板几何校验（宽度比、侧比、矩形误差） | `Detector::check_geometry(Armor)` -- config thresholds |
| DET-07 | 装甲板 ROI 提取（get_pattern） | `Detector::get_pattern()` -- cv::Rect ROI crop |
| DET-08 | 装甲板共用灯条去重逻辑 | Pair-deduplication loop, removes `duplicated` armors |
| DET-09 | Debug 可视化（标注灯条/装甲板信息） | `Detector::show_result()` -- uses `cv::imshow`, `tools::draw_text`, `tools::draw_points` |
| CLS-01 | 基于灯条几何特征的装甲板类型判断 | `Detector::get_type()` -- ratio thresholds + name-based fallback |
| CLS-02 | 基于名字的类型校验 | `Detector::check_type()` -- hero/base must be big armor |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Image preprocessing + contour extraction | API / Backend | -- | Runs on CPU frame-by-frame, no UI needed |
| Lightbar geometric validation | API / Backend | -- | Pure math on detected contours |
| Lightbar color classification | API / Backend | -- | Inline BGR channel comparison |
| Armor pair matching + validation | API / Backend | -- | Geometry rules on lightbar pairs |
| Armor ROI digit classification | API / Backend | -- | OpenVINO inference with compiled model |
| Debug visualization (imshow) | API / Backend | Browser / Client | imshow requires display (WSLg / X server / physical display) |
| Low-confidence pattern saving | API / Backend | -- | cv::imwrite to filesystem |

**Key insight:** All processing is single-threaded, frame-bounded, and runs on the CPU (OpenVINO uses CPU/AUTO device). There is no client-tier rendering in production -- debug visualization is an optional developer aid guarded by `debug_` flag.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| OpenCV | 4.x (system) | Image processing, contour detection, drawing | Ecosystem standard for CV |
| OpenVINO | 2024.6+ (system) | ONNX model inference for digit classification | D-07 locked decision; source uses `ov::Core` + `ov::CompiledModel` |
| toml++ | 3.4.0 (in `tools/tomlpp.hpp`) | Config file parsing | D-02 locked; already header-only in project |
| fmt | 10.x+ (system or FetchContent) | String formatting for debug output, save filenames | D-04/D-05 locked; source uses `fmt::format()` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Eigen3 | 3.4 (system) | Armor pose vectors (already used in armor.hpp) | Already linked; Detector uses Eigen types via Armor |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| toml++ (header-only) | cpptoml, toml11 | toml++ is already in the tree as `tools/tomlpp.hpp` -- zero build overhead |
| fmt (system) | std::format (C++20) | D-04 explicitly keeps fmt; std::format format string differences could break existing calls |

**Version verification:**
```bash
# tomlpp.hpp: toml++ v3.4.0 -- confirmed in file header [VERIFIED: source header]
# OpenCV: find_package handled by top-level CMake [VERIFIED: CMakeLists.txt]
```

**Installation:**
```bash
# OpenVINO -- add Intel apt repo, then install
wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB | sudo gpg --dearmor --output /usr/share/keyrings/intel-sw.gpg
echo "deb [signed-by=/usr/share/keyrings/intel-sw.gpg] https://apt.repos.intel.com/openvino/2024 ubuntu22 main" | sudo tee /etc/apt/sources.list.d/intel-openvino.list
sudo apt update
sudo apt install openvino-2024.6.0

# fmt -- used by source code, add to top-level CMakeLists.txt
sudo apt install libfmt-dev
# or add via FetchContent in CMake
```

## Architecture Patterns

### System Architecture Diagram

```
                    +-----------+
                    | bgr_img   |  (cv::Mat from camera)
                    +-----+-----+
                          |
                          v
              +-----------+
              | Preprocess|  cv::cvtColor -> gray
              |           |  cv::threshold -> binary
              +-----+-----+
                    |
        +-----------+-----------+
        |                       |
        v                       v
  +-----------+          +------------+
  | findContours           cv::imshow  |  (debug only)
  | + minAreaRect          ("binary_img")
  +-----------+          +------------+
        |
        v (one per contour)
  +-----------+
  | check_geom|  (angle, ratio, length thresholds)
  | (lightbar)|
  +-----------+
        |
   (pass) |
        v
  +-----------+
  | get_color |  (B vs R channel comparison)
  +-----------+
        |
        v
  +-----------+  (sorted by center.x)
  | lightbars |
  +-----------+  (std::list<Lightbar>)
        |
        v (double loop over sorted pairs)
  +-----------+
  | check_geom|  (ratio, side_ratio, rectangular_error thresholds)
  | (armor)   |
  +-----------+
        |
   (pass) |
        v
  +-----------+
  | get_pattern |  => armor.pattern (ROI crop)
  +-----------+
        |
        v
  +--------------+
  | Classifier   |  cv::dnn::Net (classify) OR ov::CompiledModel (ovclassify)
  | classify()   |
  +--------------+  => armor.name, armor.confidence
        |
        v
  +-----------+  check_name(armor) -> confidence > min_confidence, name != not_armor
  | check_name |
  +-----------+  (low-confidence patterns saved via save() for classifier training iteration)
        |
   (pass) |
        v
  +----------+
  | get_type |  => armor.type (big/small) -- ratio threshold or name-based
  +----------+
        |
        v
  +----------+
  |check_type|  hero/base must be big armor; small name can't be big
  +----------+
        |
   (pass) |
        v
  +--------------+
  | get_center_   | => armor.center_norm (normalized [0,1] coords)
  | norm         |
  +--------------+
        |
        v  (accumulate into armors list)
  +----------------------------+
  | Deduplication (shared      |  (lightbar ID overlap check)
  | lightbar IDs)              |  -> marks Armor::duplicated = true
  +----------------------------+
        |
        v
  +----------------------+
  | remove_if(duplicated)|  -> final armors list
  +----------------------+
        |
        v
  +-------------+
  | show_result |  (debug only: annotate bgr_img with text + points, cv::imshow)
  +-------------+
        |
        v
   return armors (std::list<Armor>)
```

### Recommended Project Structure

```
app/auto_aim/
  +-- detector.hpp          # Detector class declaration (NEW in this phase)
  +-- detector.cpp          # Detector implementation (NEW in this phase)
  +-- classifier.hpp        # Classifier class declaration (NEW in this phase)
  +-- classifier.cpp        # Classifier implementation (NEW in this phase)
  +-- armor.hpp             # Already exists from Phase 2
  +-- armor.cpp             # Already exists from Phase 2
  +-- auto_aim.hpp          # Already exists (empty header)
  +-- auto_aim.cpp          # Already exists (empty)
```

### Pattern 1: YAML-to-TOML Config Porting

**What:** Replace `YAML::LoadFile(config_path)` + `yaml["key"].as<double>()` with `toml::parse_file(config_path)` + `config["key"].value_or<double>(default)`.

**When to use:** All config read sites in Detector and Classifier constructors.

**Example (detector constructor):**
```cpp
// Source (yaml-cpp):
auto yaml = YAML::LoadFile(config_path);
threshold_ = yaml["threshold"].as<double>();

// Target (toml++):
auto config = toml::parse_file(config_path);
threshold_ = config["threshold"].value_or<double>(150.0);
```

The corresponding TOML config would look like:
```toml
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

[classifier]
classify_model = "models/armor_classifier.onnx"
```

### Pattern 2: Logger Call Replacement

**What:** Source uses `tools::logger()->debug(...)` with `fmt::format(...)`. Robocore uses `LOG_DEBUG(module, fmt, ...)` with `std::format(...)`. Direct `fmt::format()` calls in string building stay as-is.

**When to use:** All logger calls in detector.cpp/classifier.cpp.

**Example:**
```cpp
// Source:
tools::logger()->debug("See pattern 5");

// Target:
LOG_DEBUG(MODULE, "See pattern 5");
```

But direct fmt formatting stays:
```cpp
// Source (keep as-is):
auto file_name = fmt::format("{:%Y-%m-%d_%H-%M-%S}", std::chrono::system_clock::now());
auto img_path = fmt::format("{}/{}_{}.jpg", save_path_, armor.name, file_name);

// Target (same -- D-05 locked):
auto file_name = fmt::format("{:%Y-%m-%d_%H-%M-%S}", std::chrono::system_clock::now());
auto img_path = fmt::format("{}/{}_{}.jpg", save_path_, armor.name, file_name);
```

### Pattern 3: Namespace + Module Tag

**What:** Source uses `auto_aim::Detector`, Robocore uses `app::auto_aim::Detector`. Each .cpp needs a MODULE constant for the logging macros.

**When to use:** All files in the auto_aim module.

**Example:**
```cpp
// At top of each .cpp:
#include "app/auto_aim/detector.hpp"  // note: includes use app/ prefix

static constexpr const char* MODULE = "DETECTOR";  // D-13

namespace app::auto_aim {
// ... implementation ...
}
```

### Anti-Patterns to Avoid

- **Leaving yaml-cpp includes:** The source includes `<yaml-cpp/yaml.h>` which must be removed. yaml-cpp is not a dependency of Robocore and will fail to compile.
- **Using bare `toml::parse_file()` without try-catch:** `parse_file` throws `toml::parse_error` on invalid files. Wrap in try-catch or let top-level handler catch.
- **LOG_DEBUG with fmt::format nesting:** LOG_XXX macros use `std::format` internally. Do NOT pass `fmt::format()` results as the format string argument -- use direct format string + args: `LOG_DEBUG("DETECTOR", "value: {}", x)`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TOML parsing | Custom tokenizer | `tools/tomlpp.hpp` (toml++ v3.4.0) | TOML spec has edge cases (inline tables, multi-line strings, date/time); toml++ is production-grade |
| ONNX inference | Custom model loader | OpenVINO `ov::Core` + `ov::CompiledModel` | OpenVINO handles device selection, graph optimization, precision management |
| Logging | printf / cerr | `LOG_XXX` macros | Robocore already provides structured logging with level filtering, file/line info |

**Key insight:** The three main "don't build your own" items here are already locked decisions from CONTEXT.md. The research confirms they are the correct choices.

## Common Pitfalls

### Pitfall 1: toml++ value_or Type Mismatch
**What goes wrong:** `config["key"].value_or(0)` returns `int64_t`, but the variable is `double`. Silent truncation or compile error depending on context.

**Why it happens:** toml++ uses template deduction; integer literals default to `int64_t`.

**How to avoid:** Always explicitly specify the type or literal: `.value_or<double>(0.0)` not `.value_or(0)`.

**Warning signs:** Unexpected 0 values for threshold params, compiler warnings about narrowing conversion.

### Pitfall 2: toml::parse_file Path Resolution
**What goes wrong:** `toml::parse_file("config/test.toml")` fails when the working directory is not the project root.

**Why it happens:** Source used `YAML::LoadFile(config_path)` with the same path convention. toml++'s `parse_file` is equivalent in behavior -- the path is relative to the process working directory.

**How to avoid:** Pass absolute or project-root-relative paths consistently. The same convention from source (config_path passed as constructor arg) carries over unchanged.

**Warning signs:** `toml::parse_error` at runtime.

### Pitfall 3: OpenVINO CMake Not Auto-Detected
**What goes wrong:** `find_package(OpenVINO REQUIRED)` fails because OpenVINO's cmake config is not in the default search path.

**Why it happens:** APT-installed OpenVINO puts CMake configs at `/opt/intel/openvino_2024/runtime/cmake/`, which is not on CMake's default search path.

**How to avoid:** Source `setupvars.sh` before building, or set `OpenVINO_DIR` explicitly in the top-level CMakeLists.txt (as the source project did: `set(OpenVINO_DIR "/opt/intel/openvino_2024.6.0/runtime/cmake")`).

**Warning signs:** CMake error: `Could not find a package configuration file provided by "OpenVINO"`.

### Pitfall 4: cv::imshow Crash in Headless Environment
**What goes wrong:** `cv::imshow()` throws a GTK/Win32 error or crashes when there is no display (SSH, WSL without WSLg/X server).

**Why it happens:** OpenCV requires a GUI backend (GTK, Qt) at runtime. Without a display server, `imshow` fails.

**How to avoid:** The `debug_` flag (D-11) already guards `show_result()`. As long as `debug_` is `false` in headless deployment, `imshow` is never called. No code changes needed.

**Warning signs:** `error: (-2:Unspecified error) The function is not implemented` at runtime.

### Pitfall 5: LOG_XXX + fmt::format Conflict
**What goes wrong:** `LOG_DEBUG(MODULE, fmt::format("...", x))` compiles but produces wrong output.

**Why it happens:** `LOG_DEBUG` calls `std::format(fmt, ...)`. If `fmt` is already a formatted string (from `fmt::format`), the args won't match.

**How to avoid:** Use raw format strings with LOG_XXX: `LOG_DEBUG(MODULE, "value: {}", x)`. Only use `fmt::format()` for non-log string building (save paths, etc.).

## Code Examples

### Example 1: Classifier TOML Config (constructor migration)
```cpp
// Source (sp_vision_25/classifier.cpp):
#include <yaml-cpp/yaml.h>
// ...
Classifier::Classifier(const std::string & config_path)
{
  auto yaml = YAML::LoadFile(config_path);
  auto model = yaml["classify_model"].as<std::string>();
  net_ = cv::dnn::readNetFromONNX(model);
  auto ovmodel = core_.read_model(model);
  compiled_model_ = core_.compile_model(
    ovmodel, "AUTO", ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY));
}

// Target (Robocore):
#include "tools/tomlpp.hpp"
// ...
Classifier::Classifier(const std::string & config_path)
{
  auto config = toml::parse_file(config_path);
  auto model = config["classifier"]["classify_model"].value_or<std::string>("");
  net_ = cv::dnn::readNetFromONNX(model);
  auto ovmodel = core_.read_model(model);
  compiled_model_ = core_.compile_model(
    ovmodel, "AUTO", ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY));
}
```

### Example 2: Detector Constructor with TOML (value_or with explicit types)
```cpp
// Target -- all parameters with explicit type annotations
#include "tools/tomlpp.hpp"

Detector::Detector(const std::string & config_path, bool debug)
: classifier_(config_path), debug_(debug)
{
  auto config = toml::parse_file(config_path);

  threshold_           = config["detector"]["threshold"].value_or<double>(150.0);
  max_angle_error_     = config["detector"]["max_angle_error"].value_or<double>(15.0) / 57.3;
  min_lightbar_ratio_  = config["detector"]["min_lightbar_ratio"].value_or<double>(1.5);
  max_lightbar_ratio_  = config["detector"]["max_lightbar_ratio"].value_or<double>(10.0);
  min_lightbar_length_ = config["detector"]["min_lightbar_length"].value_or<double>(10.0);
  min_armor_ratio_     = config["detector"]["min_armor_ratio"].value_or<double>(1.0);
  max_armor_ratio_     = config["detector"]["max_armor_ratio"].value_or<double>(4.0);
  max_side_ratio_      = config["detector"]["max_side_ratio"].value_or<double>(2.0);
  min_confidence_      = config["detector"]["min_confidence"].value_or<double>(0.5);
  max_rectangular_error_ = config["detector"]["max_rectangular_error"].value_or<double>(20.0) / 57.3;

  save_path_ = "patterns";
  std::filesystem::create_directory(save_path_);
}
```

### Example 3: Logger Migration
```cpp
// Source:
tools::logger()->debug(
  "see strange armor: {} {}", ARMOR_TYPES[armor.type], ARMOR_NAMES[armor.name]);

// Target:
LOG_DEBUG(MODULE, "see strange armor: {} {}",
          ARMOR_TYPES[static_cast<int>(armor.type)],
          ARMOR_NAMES[static_cast<int>(armor.name)]);
```

Note: `ARMOR_TYPES` and `ARMOR_NAMES` are `const std::vector<std::string>`, so indexing requires `int` or `std::size_t`. Source uses implicit conversion which works but may generate warnings. Cast explicitly in the target.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| yaml-cpp config reading | toml++ value_or pattern | This phase | No runtime impact; config files change format |
| `tools::logger()->debug(...)` with `fmt::format` | `LOG_DEBUG(module, ...)` with `std::format` | This phase | Logger internal change only; direct `fmt::format()` calls preserved |
| OpenVINO 2024.6.0 (source) | Same version | N/A | Must set `OpenVINO_DIR` to match installed version path |

**Deprecated/outdated:**
- `lightbar_points_corrector` (PCA-based corner correction): enabled by D-16/D-17 to not migrate. The source already disabled it (commit 583d50a). Dead code.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `std::filesystem::create_directory(save_path_)` works consistently on the target platform | Code Examples / Detector ctor | Low risk; C++17 standard filesystem, WSL/Linux both support it |
| A2 | The `debug_` flag alone is sufficient to prevent cv::imshow crashes in headless env | Common Pitfalls / Pitfall 4 | LOW risk; if code paths other than `show_result()` call imshow, headless would crash. Audit confirms only `show_result` and `detect()` line 42 call imshow |
| A3 | toml++ v3.4.0 in `tools/tomlpp.hpp` is C++17 compatible with `parse_file` | Code Examples | HIGH confidence -- confirmed in header comments and existing usage in `read_toml.cpp` |
| A4 | ARMOR_TYPES and ARMOR_NAMES indexing needs explicit cast | Code Examples | MEDIUM -- depends on compiler warning level; source used implicit `enum` to `std::size_t` conversion |

## Open Questions

1. **What OpenVINO version should the CMake path target?**
   - What we know: Source uses OpenVINO 2024.6.0 at `/opt/intel/openvino_2024.6.0/runtime/cmake`. The apt repo may offer a newer version (2025.x).
   - What's unclear: Whether we install the exact same version or a newer one.
   - Recommendation: Install via apt and pin to the version available, then update `OpenVINO_DIR` in CMake to match. If apt offers 2025.x, source `setupvars.sh` to discover the path, or use `find_package(OpenVINO)` without setting `OpenVINO_DIR` if apt install puts it in the standard cmake path.

2. **Config file path convention -- absolute or relative?**
   - What we know: Source passes config_path as a constructor arg. Phase 7 will create the actual config file at `config/auto_aim.toml`.
   - What's unclear: Whether the config path is hardcoded or passed from the caller.
   - Recommendation: Keep the same interface as source (`Detector(std::string config_path, bool debug)`). The caller determines the path. Phase 7 will document the expected path.

3. **fmt library availability: apt vs FetchContent vs submodule?**
   - What we know: The project needs `fmt::fmt` for `fmt::format()` calls. System may or may not have `libfmt-dev`.
   - What's unclear: Preferred approach for Robocore project -- system package or bundled.
   - Recommendation: Use `FetchContent` in the top-level CMakeLists.txt if system package is not guaranteed. This avoids forcing developers to install system packages and ensures version consistency. The source project used `find_package(fmt REQUIRED)` via CMake config, suggesting system package was available.

## Environment Availability

> Phase 3 has external dependencies: OpenVINO runtime, fmt library.

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| OpenVINO | Classifier (ovclassify) | Not installed | -- | None -- Classifier requires OpenVINO (D-07 locked) |
| fmt library | Detector (save, show_result) | Not installed | -- | Use `FetchContent` or `add_subdirectory` to compile from source |
| OpenCV | Detector, Classifier | Installed | 4.x | -- |
| Eigen3 | armor.hpp (via Armor struct) | Installed | 3.4 | -- |
| yaml-cpp | (was in source) | Not needed | -- | Removed per D-03 |

**Missing dependencies with no fallback:**
- OpenVINO runtime -- must be installed via apt before building. Add apt repo setup to Phase 1 build environment tasks, or include as Phase 3 prerequisite.

**Missing dependencies with fallback:**
- fmt library -- can use `FetchContent(https://github.com/fmtlib/fmt.git)` in CMake if system package is not installed.

## Validation Architecture

> `workflow.nyquist_validation` key is absent from config -- treat as enabled.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None yet -- Phase 7 will create test program |
| Config file | None |
| Quick run command | N/A this phase |
| Full suite command | `cmake --build <build-dir>` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DET-01 | Preprocessing pipeline | integration | Planned in Phase 7 | N/A |
| DET-02 | Contour extraction | integration | Planned in Phase 7 | N/A |
| DET-05 | Armor pair matching | integration | Planned in Phase 7 | N/A |
| DET-08 | Deduplication | integration | Planned in Phase 7 | N/A |
| CLS-01 | Type classification | integration | Planned in Phase 7 | N/A |

### Sampling Rate
- **Per task commit:** Full build
- **Per wave merge:** Full build
- **Phase gate:** Compilation passes with all new files linked

### Wave 0 Gaps
- [ ] No test infrastructure yet -- testing deferred to Phase 7 as per ROADMAP.md
- [ ] Verification for this phase: compile-only (catch linker errors, include path issues)

## Security Domain

> `security_enforcement` key is absent from config -- treat as enabled.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | yes | Input is camera frames (cv::Mat) -- validated by OpenCV bounds checking in ROI extraction |
| V6 Cryptography | no | No cryptographic operations in this phase |

### Known Threat Patterns for {stack}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Path traversal in model file path | Tampering | `classify_model` path from config; config file is trusted (local file) |
| cv::Rect bounds violation | DoS | Bounds checks in detect(Armor&) overload: `boundingBox.x < 0 || ...` and `armor_roi.empty()` guard |

## Sources

### Primary (HIGH confidence)
- `sp_vision_25/tasks/auto_aim/detector.cpp` -- full algorithm source, confirmed pattern structure [VERIFIED: file read]
- `sp_vision_25/tasks/auto_aim/classifier.cpp` -- full classifier source with OpenVINO API calls [VERIFIED: file read]
- `tools/tomlpp.hpp` -- toml++ v3.4.0 header-only, available in project [VERIFIED: file read + version comment]
- `config/testconfig.toml` + `task/test/read_toml.cpp` -- existing toml++ usage pattern in Robocore [VERIFIED: file read]
- `tools/logger.hpp` -- LOG_XXX macro signatures (uses `std::format` not `fmt::format`) [VERIFIED: file read]
- `app/auto_aim/armor.hpp` -- existing Lightbar/Armor data structures (Phase 2 complete) [VERIFIED: file read]
- `app/CMakeLists.txt` -- current build config for auto_aim static lib [VERIFIED: file read]

### Secondary (MEDIUM confidence)
- OpenVINO apt install: Intel provides apt repo at `apt.repos.intel.com/openvino` for Ubuntu 22.04 [VERIFIED: web search cross-referenced with source project CMake path convention]
- OpenVINO CMake: `find_package(OpenVINO REQUIRED)` + `target_link_libraries(... openvino::runtime)` [VERIFIED: source project CMake + web search confirmation]
- `cv::imshow` headless: Requires WSLg (Win11) or X server (Win10) in WSL; guarded by `debug_` flag [VERIFIED: web search]

### Tertiary (LOW confidence)
- fmt library via apt: `apt install libfmt-dev` -- assumed available; fallback to FetchContent recommended [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries verified in project files or official sources
- Architecture: HIGH -- full algorithm flow read from source code
- Pitfalls: MEDIUM -- OpenVINO install path depends on exact version installed; fmt availability depends on system state

**Research date:** 2026-05-01
**Valid until:** 2026-06-01 (standard stack stable; OpenVINO version paths may change)
