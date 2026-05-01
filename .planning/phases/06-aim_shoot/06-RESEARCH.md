# Phase 6: 弹道预测与瞄准 - Research

**Researched:** 2026-05-02
**Domain:** Ballistic trajectory, aim point selection, firing decision
**Confidence:** HIGH

## Summary

Phase 6 ports three modules from `sp_vision_25`: Trajectory (pure ballistic math, `tools/`), Aimer (aim point selection with trajectory iteration, `app/auto_aim/`), and Shooter (firing condition decision, `app/auto_aim/`). All three depend only on code already ported in Phases 2 and 5 (math_tools, target, trajectory). No new external dependencies are required.

The adaptation is primarily mechanical: namespace changes, YAML-to-TOML config, and logger macro migration. The one structural change is that `ShootMode` moves from `io::cboard.hpp` into `app/auto_aim/command.hpp` as an unscoped enum (consistent with existing `app::auto_aim` enum style). The `Command` struct loses the `horizon_distance` field (UAV-specific, per D-03).

Trajectory confirmation: zero special dependencies. Header includes `<cmath>` only. Implementation uses `std::sqrt`, `std::atan`, `std::cos`. No Eigen, no logger, no io. Purely mechanical copy with guard-rename.

**Primary recommendation:** Two plans -- (1) Create command.hpp and Trajectory, update CMake; (2) Create Aimer + Shooter with TOML config, update CMake.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- D-01: Command and ShootMode defined in `app/auto_aim/command.hpp`
- D-02: namespace `app::auto_aim` for Command/ShootMode
- D-03: Remove `horizon_distance` field from Command
- D-04: Trajectory in `tools/trajectory.hpp/.cpp`, namespace `tools`, no changes
- D-05: Trajectory namespace is `tools` -- no changes needed
- D-06: Aimer in `app/auto_aim/aimer.hpp/.cpp`
- D-07: Aimer namespace `app::auto_aim`
- D-08: Aimer config YAML -> TOML
- D-09: Aimer `tools::logger()` -> `LOG_XXX` macros
- D-10: Shooter in `app/auto_aim/shooter.hpp/.cpp`
- D-11: Shooter namespace `app::auto_aim`
- D-12: Shooter config YAML -> TOML

### Claude's Discretion
None specified.

### Deferred Ideas (OUT OF SCOPE)
None.
</user_constraints>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Ballistic trajectory solve | Tools | -- | Pure math function (quadratic projectile), no side effects |
| Aim point selection | Application | -- | Reads EKF state from Target module, produces gimbal yaw/pitch |
| Trajectory iteration | Application | -- | Convergence loop over trajectory + target prediction |
| Firing decision | Application | -- | Compares command stability, distance, and auto-fire flag |
| Config loading | Application | -- | Aimer and Shooter read TOML from config file path |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Eigen3 | 3.4+ | Vector3d, VectorXd, MatrixXd | Already in tools and auto_aim libraries; Aimer passes Eigen types from Target EKF |
| math_tools | (port) | `limit_rad`, `square`, `delta_time` | Already in tools library; Aimer uses `limit_rad`, `delta_time`, `square` |
| Trajectory | (port) | Projectile motion solver | Pure math, no dependencies beyond `<cmath>` |

### Config
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| toml++ (tomlpp) | (vendored) | TOML parsing | Already established pattern in solver/detector/tracker |

**Version verification:**
```bash
# Eigen3 -- available as system package, already used in tools and auto_aim libraries
# toml++ -- vendored as tools/tomlpp.hpp, no external version tracking needed
```

## Architecture Patterns

### System Architecture Diagram

```
Target EKF (Phase 5) ──> Aimer::aim() ──> Command (yaw, pitch, control)
                              │
                              ├── choose_aim_point()
                              │       └── reads ekf_x, armor_xyza_list from Target
                              │
                              ├── Trajectory(d, h, v0) ──> fly_time, pitch
                              │       └── converges in ≤10 iterations
                              │
                              └── predict(future) on Target for each iteration
                                       │
                                       v
                                    Shooter::shoot()
                                       │
                                       └── decides: fire or not based on:
                                            - command stability
                                            - gimbal alignment
                                            - auto_fire flag
                                            - aim point validity
```

### Recommended Project Structure
```
tools/
├── trajectory.hpp     # [NEW] Header: tools::Trajectory struct
├── trajectory.cpp     # [NEW] Implementation: projectile solve

app/auto_aim/
├── command.hpp        # [NEW] Header: Command struct + ShootMode enum (app::auto_aim)
├── aimer.hpp          # [NEW] Header: app::auto_aim::Aimer class
├── aimer.cpp          # [NEW] Implementation: aim point + trajectory iteration
├── shooter.hpp        # [NEW] Header: app::auto_aim::Shooter class
├── shooter.cpp        # [NEW] Implementation: firing condition decision
```

### Pattern 1: TOML Config Loading (Aimer)
**What:** Aimer reads scalar parameters from TOML, converting degrees to radians where applicable.
**When to use:** All config parameters for Aimer are scalars. No arrays needed.

```cpp
// Source: established pattern in solver.cpp and detector.cpp
static constexpr const char* MODULE = "AIMER";

namespace app::auto_aim {

Aimer::Aimer(const std::string & config_path)
: left_yaw_offset_(std::nullopt), right_yaw_offset_(std::nullopt)
{
  auto config = toml::parse_file(config_path);
  yaw_offset_ = config["aimer"]["yaw_offset"].value_or<double>(0.0) / 57.3;
  pitch_offset_ = config["aimer"]["pitch_offset"].value_or<double>(0.0) / 57.3;
  comming_angle_ = config["aimer"]["comming_angle"].value_or<double>(55.0) / 57.3;
  leaving_angle_ = config["aimer"]["leaving_angle"].value_or<double>(20.0) / 57.3;
  high_speed_delay_time_ = config["aimer"]["high_speed_delay_time"].value_or<double>(0.0);
  low_speed_delay_time_ = config["aimer"]["low_speed_delay_time"].value_or<double>(0.0);
  decision_speed_ = config["aimer"]["decision_speed"].value_or<double>(7.0);
  // optional shootmode offsets
  if (config["aimer"]["left_yaw_offset"] && config["aimer"]["right_yaw_offset"]) {
    left_yaw_offset_ = config["aimer"]["left_yaw_offset"].value<double>() / 57.3;
    right_yaw_offset_ = config["aimer"]["right_yaw_offset"].value<double>() / 57.3;
    LOG_INFO(MODULE, "[Aimer] successfully loading shootmode");
  }
}
```

### Pattern 2: TOML Config Loading (Shooter)
```cpp
// Source: established pattern
static constexpr const char* MODULE = "SHOOTER";

namespace app::auto_aim {

Shooter::Shooter(const std::string & config_path)
: last_command_{false, false, 0, 0}
{
  auto config = toml::parse_file(config_path);
  first_tolerance_ = config["shooter"]["first_tolerance"].value_or<double>(3.0) / 57.3;
  second_tolerance_ = config["shooter"]["second_tolerance"].value_or<double>(2.0) / 57.3;
  judge_distance_ = config["shooter"]["judge_distance"].value_or<double>(2.0);
  auto_fire_ = config["shooter"]["auto_fire"].value_or<bool>(true);
}
}
```

### Pattern 3: Logger Macro Migration
**What:** Replace `tools::logger()->debug(...)` with `LOG_DEBUG(MODULE, ...)`, etc.
**Mapping:**
| Source | Target |
|--------|--------|
| `tools::logger()->debug(...)` | `LOG_DEBUG(MODULE, ...)` |
| `tools::logger()->info(...)` | `LOG_INFO(MODULE, ...)` |
| `tools::logger()->warn(...)` | `LOG_WARN(MODULE, ...)` |

**Header change:** Remove `#include "tools/logger.hpp"` function-call style, add `#include "tools/logger.hpp"` (macro definitions are there).

### Anti-Patterns to Avoid
- **Leaving commented-out YAML includes:** Remove `#include <yaml-cpp/yaml.h>` entirely
- **Forgetting MODULE constant:** Every .cpp in app/auto_aim/ defines `static constexpr const char* MODULE = "NAME";`

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Ballistic trajectory | Manual quadratic solve | Existing Trajectory struct | Already ported, pure math, tested with RM competition physics |
| TOML parsing | Custom parser | toml++ (tomlpp.hpp) | Vendored in tools/, used by all other modules |

**Key insight:** Trajectory is a 20-line struct with a single constructor solving the quadratic projectile equation. It has zero dependencies on project infrastructure -- pure mathematical model assuming no air resistance. It does not need Eigen, logging, or any external type.

## Common Pitfalls

### Pitfall 1: Including Wrong Command/ShootMode Namespace
**What goes wrong:** Aimer source includes `"io/cboard.hpp"` and `"io/command.hpp"` but the target project has no `io/` library.
**Why it happens:** The source project had a shared `io/` namespace for CAN communication and types. Robocore separates these concerns.
**How to avoid:** Replace both includes with a single `#include "app/auto_aim/command.hpp"`. Change `io::Command` to `app::auto_aim::Command`, `io::ShootMode` to `app::auto_aim::ShootMode`, `io::left_shoot` to `app::auto_aim::left_shoot` (unscoped enum).

### Pitfall 2: Using tools::logger() Instead of LOG_XXX
**What goes wrong:** The source uses `tools::logger()->info(...)` which requires `tools::Logger::instance()` (singleton). The target uses `LOG_INFO(MODULE, ...)` macros that wrap `tools::Logger::instance().log(...)` with file/line info.
**How to avoid:** Do a line-by-line check of aimer.cpp and shooter.cpp for every `tools::logger()->` call. There are 5 calls in aimer.cpp and 1 in shooter.cpp (plus some commented out).

### Pitfall 3: Wrong Config Section Keys or Missing TOML Flat Conversion
**What goes wrong:** Source YAML had aimer/shooter parameters at the top level (no section key). TOML needs an explicit section header like `[aimer]` and `[shooter]`.
**How to avoid:** In TOML, nest under `[aimer]` and `[shooter]` sections. Access as `config["aimer"]["yaw_offset"]`. Source YAML used `yaml["yaw_offset"]` (flat), so the adaption needs to add the section nesting.

## Code Examples

### command.hpp (NEW -- creates Command + ShootMode)
```cpp
#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
#pragma once

namespace app::auto_aim {

enum ShootMode {
  left_shoot,
  right_shoot,
  both_shoot
};

struct Command {
  bool control;
  bool shoot;
  double yaw;
  double pitch;
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_COMMAND_HPP
```

### trajectory.hpp (port -- guard/macro rename only)
```cpp
#ifndef TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
#define TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
#pragma once

namespace tools {

struct Trajectory {
  bool unsolvable;
  double fly_time;
  double pitch;  // positive = upward

  // No air resistance model
  // v0: bullet initial speed (m/s)
  // d: horizontal distance to target (m)
  // h: vertical height of target (m)
  Trajectory(const double v0, const double d, const double h);
};

}  // namespace tools

#endif  // TGU_ROBOCORE_2027_TOOLS_TRAJECTORY_HPP
```

### Aimer: Include + Namespace Changes
**Header (`aimer.hpp`):**
- Include `"app/auto_aim/command.hpp"` instead of `"io/cboard.hpp"` + `"io/command.hpp"`
- Include `"app/auto_aim/target.hpp"` instead of `"target.hpp"`
- Namespace: `namespace app::auto_aim { ... }` instead of `namespace auto_aim { ... }`
- Method signatures: `Command` instead of `io::Command`, `ShootMode` instead of `io::ShootMode`
- Forward declarations: NOT needed since all types are in `app::auto_aim`

**Implementation (`aimer.cpp`):**
- Remove `#include <yaml-cpp/yaml.h>` (entirely absent in target)
- Remove `#include "io/cboard.hpp"` + `#include "io/command.hpp"` + `#include "tools/logger.hpp"`
- Add `#include "tools/logger.hpp"` (yes, same path -- but now for macros, not the function-style API)
- Add `#include "app/auto_aim/command.hpp"` (only for side effects; compiler gets Command and ShootMode from aimer.hpp indirectly)
- Change `#include "target.hpp"` to `#include "app/auto_aim/target.hpp"`
- Change `#include "tools/trajectory.hpp"` to same path (unchanged -- tools is `tools/trajectory.hpp` in both)
- Add `static constexpr const char* MODULE = "AIMER";`
- Change `namespace auto_aim {` to `namespace app::auto_aim {`
- Change `YAML::LoadFile(config_path)` + `.as<double>()` pattern to `toml::parse_file` + `.value_or<double>()`
- Change `tools::logger()->debug(...)` to `LOG_DEBUG(MODULE, ...)` (3 calls)
- Change `tools::logger()->warn(...)` to `LOG_WARN(MODULE, ...)` (1 call)
- Change `tools::logger()->info(...)` to `LOG_INFO(MODULE, ...)` (1 call)
- Change `io::Command` to `Command` (already in scope via namespace)
- Change `io::ShootMode` to `ShootMode`
- Change `io::left_shoot` to `left_shoot`, `io::right_shoot` to `right_shoot`

### Shooter: Include + Namespace Changes
**Header (`shooter.hpp`):**
- Change `#include "io/command.hpp"` to `#include "app/auto_aim/command.hpp"`
- Change `#include "tasks/auto_aim/aimer.hpp"` to `#include "app/auto_aim/aimer.hpp"` 
- Method signatures: `Command` instead of `io::Command`

**Implementation (`shooter.cpp`):**
- Remove `#include <yaml-cpp/yaml.h>`
- Remove `#include "tools/logger.hpp"` (function-call style)
- Add `#include "tools/logger.hpp"` (macro style -- same path)
- Add `static constexpr const char* MODULE = "SHOOTER";`
- YAML to TOML parsing (see Pattern 2 above)
- `io::Command` -> `Command`
- All types already resolved via namespace

## CMake Changes Required

### tools/CMakeLists.txt
Add trajectory.cpp to the static library sources:
```
add_library(tools STATIC
    crc.cpp
    logger.cpp
    foxglove_comm.cpp
    math_tools.cpp
    img_tools.cpp
    extended_kalman_filter.cpp
    trajectory.cpp          # ADD
)
```

No new link dependencies needed. Trajectory uses only `std::sqrt`, `std::atan`, `std::cos` from `<cmath>`.

### app/CMakeLists.txt
Add aimer.cpp and shooter.cpp to the auto_aim library:
```
add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
    auto_aim/armor.cpp
    auto_aim/classifier.cpp
    auto_aim/detector.cpp
    auto_aim/solver.cpp
    auto_aim/target.cpp
    auto_aim/tracker.cpp
    auto_aim/voter.cpp
    auto_aim/aimer.cpp        # ADD
    auto_aim/shooter.cpp      # ADD
)
```

No new link dependencies needed. auto_aim already links `tools` (provides Trajectory, math_tools) and `Eigen3::Eigen`.

## Runtime State Inventory

> Not applicable -- this is a greenfield port phase with no existing runtime state. All three modules (Trajectory, Aimer, Shooter) are being created for the first time in the target project. No rename/refactor/migration involved.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `tools::logger()->debug(...)` | `LOG_DEBUG(MODULE, ...)` | Phase 6 adapter | Logger macros add file/line info automatically |
| YAML config (yaml-cpp) | TOML config (toml++) | Phase 6 adapter | No yaml-cpp dependency needed; consistent with Phases 3-5 |
| `io::Command` in `io/` namespace | `app::auto_aim::Command` | Phase 6 | Collocated with Aimer/Shooter; no `io/` library needed |
| `io::ShootMode` in `io/cboard.hpp` | `app::auto_aim::ShootMode` in `command.hpp` | Phase 6 | No CBoard dependency in Aimer per user decision |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Target `ekf_x()[8]` access in Aimer `choose_aim_point()` works identically in ported Target | Architecture Patterns | If the 11-state vector was modified during port, index 8 (radius `r`) may have shifted. Verified: ported target.cpp Line 243 uses `ekf_.x[8]` for `r` in `diverged()`, matching source target.cpp. **Status: VERIFIED by code inspection.** |
| A2 | `ArmorName::outpost` is still defined in the ported armor.hpp | Architecture Patterns | Verified: armor.hpp line 37 includes `outpost` in the `ArmorName` enum. **Status: VERIFIED by code inspection.** |
| A3 | All message format strings in LOG_XXX calls are compatible with `std::format` (used by the logger macros) | Code Examples | Source used `fmt::format` style. Target logger uses `std::format`. Both use the same `{:.2f}` syntax for floats. **Status: VERIFIED by code inspection of tools/logger.hpp using std::format.** |

## Open Questions

1. **Optional `left_yaw_offset` / `right_yaw_offset` in TOML**
   - What we know: Source checks `yaml["left_yaw_offset"].IsDefined()`. TOML equivalent is `if (config["aimer"]["left_yaw_offset"])`.
   - What's unclear: Whether these will be present in Phase 7's config file. The source's standard3.yaml does NOT define them -- they only appear in sentry configs.
   - Recommendation: Use `if (config["aimer"]["left_yaw_offset"] && config["aimer"]["right_yaw_offset"])` to check presence. Graceful fallback: if absent, `left_yaw_offset_` and `right_yaw_offset_` remain `std::nullopt`.

2. **TOML config TOML section key specification**
   - What we know: Previous phases (detector, tracker) use section-keyed TOML like `[detector]`, `[tracker]`.
   - What's unclear: Should aimer/shooter config be in a separate file or merged into the same config file as detector/tracker/solver?
   - Recommendation: Use same file as other modules (`config/auto_aim.toml`), with `[aimer]` and `[shooter]` sections. Phase 7 will create the actual config file.

## Environment Availability

> This phase has no external dependencies beyond what Phases 1-5 already established. Eigen3, OpenCV, fmt, toml++, and Boost are already installed and linked. No new tools or services are required.

## Validation Architecture

> Skipped: `workflow.nyquist_validation` is explicitly set to `false` in `.planning/config.json`.

## Security Domain

> Skipped: Phase 6 has no network, authentication, input validation, or cryptography concerns. This is a mechanical port of math + decision logic operating on in-memory Eigen types.

## Sources

### Primary (HIGH confidence) -- verified by code inspection
- `sp_vision_25/tools/trajectory.hpp/.cpp` -- Pure ballistic math, no dependencies
- `sp_vision_25/tasks/auto_aim/aimer.hpp/.cpp` -- Aimer with YAML, io::Command, tools::logger
- `sp_vision_25/tasks/auto_aim/shooter.hpp/.cpp` -- Shooter with YAML, io::Command, tools::logger
- `sp_vision_25/tasks/auto_aim/target.hpp/.cpp` -- Target API that Aimer consumes (ekf_x, armor_xyza_list, predict)
- `sp_vision_25/io/command.hpp` -- Source Command struct
- `sp_vision_25/io/cboard.hpp` -- Source ShootMode enum
- `TGU_Robocore_2027/app/auto_aim/armor.hpp` -- Target ArmorName enum (includes outpost)
- `TGU_Robocore_2027/app/auto_aim/target.hpp/.cpp` -- Target Target class (ekf_x, predict, jumped, armor_xyza_list)
- `TGU_Robocore_2027/tools/CMakeLists.txt` -- Current tools library sources
- `TGU_Robocore_2027/app/CMakeLists.txt` -- Current auto_aim library sources
- `TGU_Robocore_2027/tools/logger.hpp` -- LOG_XXX macros using std::format
- `TGU_Robocore_2027/config/testconfig.toml` -- TOML config pattern reference

### Secondary (MEDIUM confidence)
- `sp_vision_25/configs/standard3.yaml` -- Parameter values for aimer and shooter sections (used to verify defaults match)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All dependencies confirmed by code inspection of both source and target projects
- Architecture: HIGH -- Code paths traced through from Aimer calling Target::ekf_x(), Target::predict(), Trajectory constructor
- Pitfalls: HIGH -- Each pitfall documented from direct comparison of source vs. target patterns

**Research date:** 2026-05-02
**Valid until:** 2026-06-02 (stable code, no fast-moving dependencies)
