# Phase 5: EKF 目标跟踪 - Research

**Researched:** 2026-05-02
**Domain:** Extended Kalman Filter target tracking, state machine, multi-frame voting
**Confidence:** HIGH

## Summary

Phase 5 migrates four tightly-coupled modules from `sp_vision_25` to the Robocore framework: ExtendedKalmanFilter (generic EKF base class to `tools/`), Target (11-DOF EKF state management), Tracker (lost/detecting/tracking/temp_lost state machine), and Voter (armor type multi-frame voting). The source code has been fully reviewed.

**Primary recommendation:** EKF has no special dependencies beyond Eigen3 -- confirmed by reading the full implementation. The generic EKF class is a ~100-line header+source using only `Eigen/Dense` and standard library (`functional`, `deque`, `map`). The business-logic classes (Target, Tracker, Voter) depend only on already-migrated modules (armor, math_tools, solver) and require no new external libraries. The migration is mechanically straightforward: copy files, adapt includes, replace YAML config with TOML, replace `tools::logger()` with `LOG_XXX` macros, remove the omniperception overload, and comment out outpost/base specializations per CONTEXT.md decisions.

### Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Generic EKF predict/update | tools/ (library) | -- | Algorithm is robot-agnostic; no auto-aim coupling |
| 11-DOF target state estimation | app/auto_aim/ | tools/ (EKF base) | Target objects encapsulate armor-specific kinematics |
| Tracker state machine | app/auto_aim/ | -- | Depends on Solver (camera extrinsics), inherits Target |
| Armor type voting | app/auto_aim/ | -- | Pure counters on armor enums, no external deps |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Eigen3 | 3.4+ (system) | Linear algebra for EKF (matrix/vector ops) | Required by all EKF math; already in tools/ and app/ CMake targets |
| toml++ (tomlpp.hpp) | 3.4.0 | TOML config parsing | Already vendored in tools/tomlpp.hpp; used by all migrated modules |
| tools::math_tools | -- | `limit_rad`, `xyz2ypd`, `xyz2ypd_jacobian`, `delta_time` | Already migrated; Target/Tracker depend on these |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| OpenCV (CV_PI) | 4.x+ | CV_PI constant for rotation calculations | Target.cpp needs it for `2 * CV_PI / armor_num_`; transitively available via armor.hpp |
| LOG_XXX macros | -- | Replaces `tools::logger()->debug/warn` | All logging in Target and Tracker |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom EKF | OpenCV KalmanFilter | OpenCV KF is linear-only, no custom h/z_subtract support. Custom EKF is the right call. |
| Custom EKF | Boost::numeric::odeint | Overkill for discrete-time EKF; would add heavy dependency for no benefit. |

**Installation:**
```bash
# No new system packages needed.
# Eigen3 already installed from Phase 1.
# tomlpp.hpp already vendored from Phase 1.
```

**Version verification:** Eigen3 confirmed in `app/CMakeLists.txt` as `Eigen3::Eigen`. toml++ v3.4.0 confirmed via header comment in `tools/tomlpp.hpp`.

## Architecture Patterns

### System Architecture Diagram

```
                   ┌─────────────────────────────┐
                   │  Tracker state machine       │
                   │  (lost/detecting/tracking/   │
                   │   temp_lost)                  │
                   └──────────┬──────────────────┘
                              │ owns 1
                    ┌─────────▼──────────┐
                    │  Target (11-DOF)    │
                    │  - predict(dt)      │
                    │  - update(armor)    │
                    │  - diverged()       │
                    │  - convergened()    │
                    └─────────┬──────────┘
                              │ wraps 1
                    ┌─────────▼──────────────┐
                    │  ExtendedKalmanFilter  │ tools::
                    │  - predict(F, Q, f)    │
                    │  - update(z, H, R, h)  │
                    │  - NIS chi-squared     │
                    └────────────────────────┘

  External inputs:
    ┌──────────────┐     ┌─────────────┐     ┌──────────────┐
    │ Solver::solve│────>│ Armor list  │────>│ Tracker.track│
    │ (already     │     │ (from       │     │ (this phase) │
    │  migrated)   │     │  Detector)  │     └──────┬───────┘
    └──────────────┘     └─────────────┘            │
                                           ┌────────▼────────┐
                                           │  Target list     │
                                           │  (output to      │
                                           │   Phase 6 Aim)   │
                                           └─────────────────┘

    ┌──────────────┐
    │ Voter        │── counts armor type occurrences
    │ (independent │   (used externally, not by Tracker)
    └──────────────┘
```

### Recommended Project Structure
```
tools/
├── extended_kalman_filter.hpp  # NEW
├── extended_kalman_filter.cpp  # NEW

app/auto_aim/
├── target.hpp                  # NEW
├── target.cpp                  # NEW
├── tracker.hpp                 # NEW
├── tracker.cpp                 # NEW
├── voter.hpp                   # NEW
├── voter.cpp                   # NEW
```

### Pattern 1: TOML Config Parsing
**What:** Use `toml::parse_file()` followed by `table["key"].value_or<T>(default)` for config.
**When to use:** All new config files; matches existing Detector/Classifier/Solver pattern.
**Example:**
```cpp
#include "tools/tomlpp.hpp"

auto config = toml::parse_file(config_path);
min_detect_count_ = config["tracker"]["min_detect_count"].value_or<int>(5);
max_temp_lost_count_ = config["tracker"]["max_temp_lost_count"].value_or<int>(50);
```

### Pattern 2: LOG_XXX Macros
**What:** Replace `tools::logger()->debug(fmt, args...)` with `LOG_DEBUG(MODULE, fmt, args...)`.
**When to use:** All logging in Target, Tracker.
**Example:**
```cpp
static constexpr const char* MODULE = "TRACKER";

// Before: tools::logger()->warn("[Tracker] Large dt: {:.3f}s", dt);
// After:
LOG_WARN(MODULE, "[Tracker] Large dt: {:.3f}s", dt);

// Before: tools::logger()->debug("[Target] r={:.3f}, l={:.3f}", r, l);
// After:
LOG_DEBUG(MODULE, "[Target] r={:.3f}, l={:.3f}", ekf_.x[8], ekf_.x[9]);
```

### Pattern 3: Image Center Normalized Coordinates
**What:** Replace hardcoded `img_center(1440/2, 1080/2)` with `center_norm` field from Armor.
**When to use:** Tracker armor sorting by screen center distance.
**Example:**
```cpp
// Before:
armors.sort([](const Armor & a, const Armor & b) {
    cv::Point2f img_center(1440 / 2, 1080 / 2);
    auto distance_1 = cv::norm(a.center - img_center);
    auto distance_2 = cv::norm(b.center - img_center);
    return distance_1 < distance_2;
});

// After:
armors.sort([](const Armor & a, const Armor & b) {
    cv::Point2f center_norm(0.5, 0.5);
    auto distance_1 = cv::norm(a.center_norm - center_norm);
    auto distance_2 = cv::norm(b.center_norm - center_norm);
    return distance_1 < distance_2;
});
```

### Anti-Patterns to Avoid
- **Forgetting to remove omniperception includes:** tracker.hpp includes `tasks/omniperception/perceptron.hpp` and `tools/thread_safe_queue.hpp` -- these are only needed by the second `track()` overload which is being removed. Remove both includes and the overload.
- **Forgetting `pragma once`:** All Robocore headers use `#pragma once` alongside the include guard. New files should follow this convention.
- **Using old include path style:** Robocore uses project-root-relative includes (`"app/auto_aim/armor.hpp"`), not local-relative (`"armor.hpp"`) or `sp_vision_25` style (`"tasks/auto_aim/armor.hpp"`).
- **Leaving `using namespace`:** Target/Tracker/Voter in Robocore use explicit `namespace app::auto_aim { ... }` wrapping, not `using namespace`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| TOML parsing | Custom parser | tools/tomlpp.hpp (toml++ v3.4.0) | Standard single-header library, already vendored |
| Logging | `std::cout` or custom prints | LOG_XXX macros | Integrated with Robocore logger (levels, file output, debug compile-time removal) |
| Matrix math | Hand-rolled matrices | Eigen3 | Vectorized, tested, already a project dependency |

**Key insight:** All four modules being migrated already use the right libraries. The migration is about adapting namespace/include/config conventions, not about replacing dependencies.

## Common Pitfalls

### Pitfall 1: YAML to TOML Tracker Config
**What goes wrong:** The original Tracker uses `YAML::LoadFile()` with `.as<T>()`, which has different syntax and behavior from toml++.
**Why it happens:** toml++ returns `std::optional<T>` from `.value<T>()`, while yaml-cpp throws on missing keys.
**How to avoid:** Use `value_or<T>(default)` for every config key. Required keys that have no sensible default should use `value<T>()` (which returns `std::optional<T>`) and check with an explicit error message.
**Warning signs:** Build errors from `toml::parse_file` returning a different type, or `.as<T>()` not existing.

### Pitfall 2: Logger Format String Differences
**What goes wrong:** `tools::logger()->debug(fmt, args...)` used `fmt::format` syntax from fmtlib. The Robocore `LOG_DEBUG` uses `std::format` (C++20). While format syntax is nearly identical, edge cases differ.
**Why it happens:** `std::format` and `fmt::format` have subtle differences in chrono formatting and error handling.
**How to avoid:** The format strings in Target/Tracker use only basic float/int formatting (`{:.3f}`, `{}`), which is identical between both. No changes to format specifiers are needed.
**Warning signs:** Compile errors on `std::format` for complex format strings, or runtime format errors.

### Pitfall 3: CV_PI Availability
**What goes wrong:** `CV_PI` is defined in OpenCV headers but only accessible if `<opencv2/opencv.hpp>` or `<opencv2/core.hpp>` is included.
**Why it happens:** `target.cpp` uses `CV_PI` for rotation angle math (`id * 2 * CV_PI / armor_num_`).
**How to avoid:** `app/auto_aim/armor.hpp` includes `<opencv2/opencv.hpp>`, so any file including `armor.hpp` has `CV_PI` available transitively. Target.hpp includes `armor.hpp`, so this is automatically satisfied.
**Warning signs:** Compile error `'CV_PI' was not declared in this scope`.

### Pitfall 4: Include Path Adjustments
**What goes wrong:** The source files use `#include "armor.hpp"` (same-directory relative), but Robocore convention uses `#include "app/auto_aim/armor.hpp"` (project-root relative for app modules) or `#include "tools/xxx.hpp"` for tools.
**How to avoid:** Update all includes:
- `"armor.hpp"` -> `"app/auto_aim/armor.hpp"`
- `"solver.hpp"` -> `"app/auto_aim/solver.hpp"`
- `"tools/extended_kalman_filter.hpp"` -> `"tools/extended_kalman_filter.hpp"` (already correct since tools headers use project-root path)
- `"tools/logger.hpp"` -> `"tools/logger.hpp"`
- `"tools/math_tools.hpp"` -> `"tools/math_tools.hpp"`
**Warning signs:** Build errors with 'file not found'.

## Code Examples

### ExtendedKalmanFilter (tools/, generic, no changes needed)

```cpp
// Source: reviewed from sp_vision_25/tools/extended_kalman_filter.hpp/.cpp
namespace tools {

class ExtendedKalmanFilter {
public:
  Eigen::VectorXd x;
  Eigen::MatrixXd P;

  ExtendedKalmanFilter(
    const Eigen::VectorXd & x0, const Eigen::MatrixXd & P0,
    std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> x_add =
      [](const Eigen::VectorXd & a, const Eigen::VectorXd & b) { return a + b; });

  Eigen::VectorXd predict(const Eigen::MatrixXd & F, const Eigen::MatrixXd & Q);

  Eigen::VectorXd predict(
    const Eigen::MatrixXd & F, const Eigen::MatrixXd & Q,
    std::function<Eigen::VectorXd(const Eigen::VectorXd &)> f);

  Eigen::VectorXd update(
    const Eigen::VectorXd & z, const Eigen::MatrixXd & H, const Eigen::MatrixXd & R,
    std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> z_subtract =
      [](const Eigen::VectorXd & a, const Eigen::VectorXd & b) { return a - b; });

  Eigen::VectorXd update(
    const Eigen::VectorXd & z, const Eigen::MatrixXd & H, const Eigen::MatrixXd & R,
    std::function<Eigen::VectorXd(const Eigen::VectorXd &)> h,
    std::function<Eigen::VectorXd(const Eigen::VectorXd &, const Eigen::VectorXd &)> z_subtract =
      [](const Eigen::VectorXd & a, const Eigen::VectorXd & b) { return a - b; });

  std::map<std::string, double> data;  // NIS/NEES diagnostic data
  std::deque<int> recent_nis_failures{0};
  size_t window_size = 100;
  double last_nis;

private:
  Eigen::MatrixXd I;
  // ...
};

}  // namespace tools
```

### Target 11-DOF State Transition Matrix
```cpp
// Source: reviewed from sp_vision_25/tasks/auto_aim/target.cpp
// State: [x, vx, y, vy, z, vz, angle, angular_vel, radius, r_diff, z_diff]
Eigen::MatrixXd F{
  {1, dt,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  1, dt,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  1, dt,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  1, dt,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1}
};
```

### Tracker State Machine
```cpp
// Source: reviewed from sp_vision_25/tasks/auto_aim/tracker.cpp
// States: lost -> detecting -> tracking -> temp_lost
void Tracker::state_machine(bool found) {
  if (state_ == "lost") {
    if (!found) return;
    state_ = "detecting";
    detect_count_ = 1;
  }
  else if (state_ == "detecting") {
    if (found) { detect_count_++; if (detect_count_ >= min_detect_count_) state_ = "tracking"; }
    else { detect_count_ = 0; state_ = "lost"; }
  }
  else if (state_ == "tracking") {
    if (found) return;
    temp_lost_count_ = 1; state_ = "temp_lost";
  }
  else if (state_ == "temp_lost") {
    if (found) { state_ = "tracking"; }
    else {
      temp_lost_count_++;
      if (temp_lost_count_ > max_temp_lost_count_) state_ = "lost";
    }
  }
}
```

### Voter (no migration changes needed)
```cpp
// Source: reviewed from sp_vision_25/tasks/auto_aim/voter.hpp/.cpp
namespace app::auto_aim {

class Voter {
public:
  Voter();
  void vote(const Color color, const ArmorName name, const ArmorType type);
  std::size_t count(const Color color, const ArmorName name, const ArmorType type);
private:
  std::vector<std::size_t> count_;
  std::size_t index(const Color color, const ArmorName name, const ArmorType type) const;
};

// index = color * (NAMES * TYPES) + name * TYPES + type
// Straight-line migration -- no changes needed.
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `tools::logger()->debug()` | `LOG_DEBUG(MODULE, ...)` | Phase 5 migration | Module-based log filtering, compile-time Debug removal in Release |
| YAML config (yaml-cpp) | TOML config (toml++) | Phase 5 migration | Standardized with all other migrated modules |
| `img_center(1440/2, 1080/2)` | `center_norm(0.5, 0.5)` | Phase 5 migration | Resolution-independent; center_norm already computed by Detector |

**Deprecated/outdated:**
- yaml-cpp library: Not used anywhere in Robocore auto_aim. TOML is the standard.
- Omniperception multi-track overload: Being removed per D-01/D-02. Future sentry mode on separate branch.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `CV_PI` is available transitively through `armor.hpp` -> `<opencv2/opencv.hpp>` | Standard Stack | Low risk -- confirmed by existing armor.hpp include chain. If removed in future OpenCV versions, `M_PI` from `<cmath>` is an alternative. |
| A2 | Voter index math works with the migrated `ArmorName`/`ArmorType`/`Color` enums | Architecture | Low risk -- enums are 1:1 migrated from sp_vision_25. The vector `count_` size matches `COLORS.size() * ARMOR_NAMES.size() * ARMOR_TYPES.size()`, which must remain in sync. |
| A3 | `std::format` format strings with `{:.3f}` work identically to `fmt::format` | Common Pitfalls | Low risk -- basic float formatting is specified identically in C++20 standard and fmtlib. |

## Open Questions

1. **(None.)** -- All module implementations have been fully reviewed. No gaps remain. The EKF dependency on Eigen3-only has been confirmed by reading the full source.

## Environment Availability

> Skipped -- Phase 5 has no new external dependencies. All dependencies (Eigen3, toml++, OpenCV, math_tools) are already available from Phases 1-2.

## Validation Architecture

> Skipped -- `workflow.nyquist_validation` is explicitly `false` in `.planning/config.json`.

## Security Domain

> Skipped -- `security_enforcement` is absent from config (default: enabled would apply). However, Phase 5 migrates internal tracking algorithms with no network exposure, no user input, and no data persistence. No ASVS categories apply.

## Sources

### Primary (HIGH confidence)
- [VERIFIED: sp_vision_25 source review] - Full reading of extended_kalman_filter.hpp/.cpp, target.hpp/.cpp, tracker.hpp/.cpp, voter.hpp/.cpp
- [VERIFIED: Robocore existing code review] - Full reading of tools/CMakeLists.txt, app/CMakeLists.txt, app/auto_aim/armor.hpp, app/auto_aim/solver.hpp/.cpp, app/auto_aim/detector.hpp/.cpp, app/auto_aim/classifier.hpp/.cpp, tools/tomlpp.hpp, tools/logger.hpp, tools/math_tools.hpp
- [VERIFIED: Context7/Eigen3 docs] - Eigen3 is header-only linear algebra library; `Eigen/Dense` provides all matrix/vector types used in EKF

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All dependencies verified by code review and existing project state
- Architecture: HIGH - Source code fully reviewed; migration changes are mechanical and well-documented
- Pitfalls: HIGH - All pitfalls identified from actual migration patterns in Phases 1-4

**Research date:** 2026-05-02
**Valid until:** Stable -- Eigen3 and toml++ are mature, unchanging dependencies
