# Phase 2: Data Structures and Tools Layer - Research

**Researched:** 2026-05-01
**Domain:** C++ data structure migration, CMake build integration
**Confidence:** HIGH

## Summary

Phase 2 migrates three data/tool modules from sp_vision_25 to TGU_Robocore_2027:
1. `armor.hpp/.cpp` (Lightbar/Armor data structures, enums, constructors)
2. `math_tools.hpp/.cpp` (mathematical utilities: angle limiting, coordinate transforms, timing)
3. `img_tools.hpp/.cpp` (OpenCV drawing helpers)

All three are pure code migrations — no new algorithms, no runtime state changes, no external service configs. The key integration points are CMake file lists (adding new source files to existing library targets) and Eigen3 linking (header-only library, already found in top-level CMake).

**Primary recommendation:** Add source files to existing CMake targets, adapt include guards and namespaces per project style, and resolve the one dependency gap (Eigen3::Eigen not yet linked to `app/auto_aim` or `tools` targets). All changes are localized to files within `tools/` and `app/auto_aim/`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** armor.hpp/.cpp in `app/auto_aim/`
- **D-02:** math_tools.hpp/.cpp in `tools/`
- **D-03:** img_tools.hpp/.cpp in `tools/`
- **D-04:** math_tools stays one file
- **D-05:** Replace spdlog with LOG_XXX macros
- **D-06:** `tools::logger()->info(...)` -> `LOG_INFO(MODULE, ...)`
- **D-07:** `tools::logger()->debug(...)` -> `LOG_DEBUG(MODULE, ...)`
- **D-08:** `tools::logger()->warn(...)` -> `LOG_WARN(MODULE, ...)`
- **D-09:** Each .cpp defines `static constexpr const char* MODULE`
- **D-10:** No spdlog dependency
- **D-11:** armor uses `namespace app::auto_aim`
- **D-12:** math_tools uses `namespace tools`
- **D-13:** img_tools uses `namespace tools`
- **D-14:** Full include paths: `#include "app/auto_aim/armor.hpp"`
- **D-15:** CMake already has `target_include_directories(... ${PROJECT_SOURCE_DIR})`
- **D-16:** Include guard: `TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP`
- **D-17:** Use `#pragma once`
- **D-18:** Remove fmt dependency
- **D-19:** Keep `<Eigen/Dense>` and `<Eigen/Geometry>` includes

### Claude's Discretion
- Function internals unchanged
- CMake integration via existing `tools/CMakeLists.txt`

### Deferred Ideas (OUT OF SCOPE)
None
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DATA-01 | Define Color, ArmorType, ArmorName, ArmorPriority enums | Direct copy from sp_vision_25, enum values and string tables unchanged. Namespace `auto_aim` -> `app::auto_aim`. |
| DATA-02 | Define Lightbar struct (center, corners, angle, aspect ratio, etc.) | Direct copy. Constructor implementation unchanged. Add `#include <tuple>` for `armor_properties` table. |
| DATA-03 | Define Armor struct (lightbar pair, type, pose, confidence, etc.) | Direct copy. 5 constructor overloads unchanged. Namespace only change. |
| DATA-04 | Support traditional vision (lightbar pair) and neural network construction | All 5 constructors preserved (traditional, NN detection, NN ROI, YOLOV5, YOLOV5+ROI). No algorithm changes. |
| MATH-01 | `limit_rad()` | Direct copy. Uses `CV_PI` from `<opencv2/core.hpp>` -- OpenCV 4.6.0 available. |
| MATH-02 | `eulers()` rotation/quaternion to euler angles | Direct copy. Uses Eigen3 `Quaterniond`, `Matrix3d` -- Eigen3 3.4.0 header-only available. |
| MATH-03 | `xyz2ypd()` / `ypd2xyz()` | Direct copy. Pure Eigen3 math, no external deps beyond Eigen3. |
| MATH-04 | `delta_time()` | Direct copy. Uses `std::chrono::steady_clock` -- standard C++20, no extra deps. |
| MATH-05 | Coordinate transform Jacobians | Direct copy. Pure Eigen3 math. |
| MATH-06 | `rotation_matrix()` | Direct copy. Pure Eigen3 math. |
| MATH-07 | `get_abs_angle()` | Direct copy. Pure Eigen3 math. |
| IMG-01 | `draw_point` / `draw_points` | Direct copy. Uses `cv::circle`, `cv::drawContours` -- OpenCV 4.6.0 available. |
| IMG-02 | `draw_text` | Direct copy. Uses `cv::putText` -- OpenCV 4.6.0 available. |
| IMG-03 | Adapt to Robocore `cv::Mat` usage | No changes needed -- `cv::Mat` usage is identical. |  | | |

</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Armor data structures | API / Backend | --- | Data types used across the auto_aim pipeline. No client-side or SSR involvement. |
| Math utilities | API / Backend (Tools) | --- | Generic math functions, no dependency on app/io/task. Used by solver, tracker, aimer in later phases. |
| Image drawing utilities | API / Backend (Tools) | --- | OpenCV-based drawing helpers. Used by Detector for debug visualization. |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Eigen3 | 3.4.0 | Linear algebra (vectors, matrices, quaternions) | Header-only, no runtime linking. Already in top-level CMake (`find_package(Eigen3 REQUIRED)`). [VERIFIED: dpkg -l] |
| OpenCV | 4.6.0 | Computer vision primitives (cv::Mat, cv::Point, CV_PI) | Already in top-level CMake (`find_package(OpenCV REQUIRED)`). [VERIFIED: dpkg -l] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| tools::Logger | current | Logging via LOG_XXX macros | When code needs to log (future phases). Phase 2 files have no log calls. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Eigen3::Eigen (header-only) | Manual include path | No benefit. `find_package(Eigen3)` is already in the build. |
| LOG_XXX macros | spdlog | Avoids adding spdlog/fmt dependency. Phase 2 files don't use logging. |

**Version verification:**
```bash
$ dpkg -l libeigen3-dev | tail -1
ii  libeigen3-dev  3.4.0-4build0.1  all  lightweight C++ template library
$ dpkg -l libopencv-dev | tail -1
ii  libopencv-dev  4.6.0+dfsg-13.1ubuntu1  amd64  development files for opencv
```

## Architecture Patterns

### System Architecture Diagram

```
                  sp_vision_25 (source)          TGU_Robocore_2027 (target)
                  ==================             =========================

  ┌─────────────────────────────┐       ┌─────────────────────────────────────┐
  │ tasks/auto_aim/armor.hpp    │       │ app/auto_aim/armor.hpp              │
  │ tasks/auto_aim/armor.cpp    │─ ─ ─ >│ app/auto_aim/armor.cpp              │
  │ namespace auto_aim          │  copy │ namespace app::auto_aim             │
  └─────────────────────────────┘  w/   │ include guard: TGU_ROBOCORE_2027_* │
                                     ad.│ #pragma once                      │
  ┌─────────────────────────────┘       └────────────┬────────────────────────┘
  │            sp_vision_25 (source)                 │
  │ tools/math_tools.hpp          │       ┌──────────┴────────────────────────┐
  │ tools/math_tools.cpp          │─ ─ ─ >│ tools/math_tools.hpp              │
  │ namespace tools               │  copy │ tools/math_tools.cpp              │
  └───────────────────────────────┘  w/   │ namespace tools                   │
                                     ad.│ include guard: TGU_ROBOCORE_2027_* │
  ┌───────────────────────────────┐       │ #pragma once                      │
  │ tasks/auto_aim/img_tools.hpp  │       └────────────┬────────────────────────┘
  │ tasks/auto_aim/img_tools.cpp  │─ ─ ─ >│             │
  │ namespace tools               │  copy │ ┌──────────┴────────────────────────┐
  └───────────────────────────────┘  w/   │ tools/img_tools.hpp                │
                                     ad.│ tools/img_tools.cpp                │
                                       │ namespace tools                    │
                                       │ include guard: TGU_ROBOCORE_2027_* │
                                       │ #pragma once                      │
                                       └────────────┬────────────────────────┘
                                                     │
                          ┌──────────────────────────┴──────────────────────────┐
                          │                     Build System                    │
                          │                                                     │
                          │  tools/CMakeLists.txt        app/CMakeLists.txt     │
                          │    math_tools.cpp               armor.cpp          │
                          │    img_tools.cpp               Eigen3::Eigen       │
                          │    +Eigen3::Eigen                                   │
                          └─────────────────────────────────────────────────────┘
```

The migration copies source files from sp_vision_25 to Robocore with namespace, include guard, and include path adaptations. CMake targets get new source files added to their existing `add_library(...)` calls.

### Recommended Project Structure (post-migration)

```
app/auto_aim/
├── auto_aim.hpp            # (existing stub, may expand later)
├── auto_aim.cpp            # (existing stub)
├── armor.hpp               # NEW: Lightbar, Armor, enums
└── armor.cpp               # NEW: constructors

tools/
├── CMakeLists.txt          # (modified: +math_tools.cpp, +img_tools.cpp, +Eigen3::Eigen)
├── logger.hpp/.cpp         # (existing)
├── crc.hpp/.cpp            # (existing)
├── math_tools.hpp          # NEW: math utility functions
├── math_tools.cpp          # NEW: implementations
├── img_tools.hpp           # NEW: OpenCV drawing functions
└── img_tools.cpp           # NEW: implementations
```

### Pattern 1: Header Adaptation Template

Every migrated header follows this pattern:
```cpp
#ifndef TGU_ROBOCORE_2027_PATH_FILE_HPP
#define TGU_ROBOCORE_2027_PATH_FILE_HPP
#pragma once

// Standard headers
#include <string>
#include <vector>

// Third-party headers
#include <Eigen/Dense>

// Project headers (full paths)
#include "app/auto_aim/some_header.hpp"

namespace app::auto_aim {

// types, structs, functions...

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_PATH_FILE_HPP
```

### Pattern 2: Source File Adaptation Template

```cpp
#include "app/auto_aim/armor.hpp"  // Full path per D-14

// Standard headers
#include <algorithm>
#include <cmath>

// Third-party headers
#include <opencv2/opencv.hpp>

// Module definition per D-09
static constexpr const char* MODULE = "AUTO_AIM";

namespace app::auto_aim {

// implementations unchanged from source

}  // namespace app::auto_aim
```

### Anti-Patterns to Avoid
- **Leaving relative includes**: Source files use `#include "armor.hpp"` -- must change to `#include "app/auto_aim/armor.hpp"` per D-14.
- **Missing explicit includes**: `armor_properties` uses `std::tuple` but doesn't include `<tuple>` (implicit via OpenCV). Robocore style requires explicit includes. Add `#include <tuple>`.
- **Omitting Eigen3 link**: `Eigen3::Eigen` must be added to both `tools` and `auto_aim` targets. Without it, compilation may fail in some build configurations despite the global `include_directories`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Linear algebra | Custom vector/matrix math | `Eigen3::Eigen` (header-only) | Already in the build system. Quaternion, rotation matrix, Jacobian operations already use Eigen. |
| Logging | std::cout or printf | `tools::Logger` macros | Project standard. Release-mode DEBUG removal, file line info, module tagging. |
| Image drawing | Pixel-level OpenCV calls | `tools::img_tools` wrappers | Already being migrated. Provides consistent drawing style across the codebase. |
| Angle wrapping | Custom radian normalization | `tools::limit_rad()` | Already being migrated. Single definition used by all modules. |

**Key insight:** All three modules being migrated already use the correct libraries (Eigen3, OpenCV, std::chrono). No replacement needed. The migration is purely about file placement, naming, and namespace conformance.

## Common Pitfalls

### Pitfall 1: Eigen3 Link Not Declared on tools Target
**What goes wrong:** `tools/math_tools.hpp` includes `<Eigen/Geometry>`. If the `tools` target does not `target_link_libraries(tools PUBLIC Eigen3::Eigen)`, any target downstream of `tools` (like `io`, `task/test_*`) will fail to find Eigen3 headers when they transitively include `math_tools.hpp`.

**Why it happens:** The top-level CMake has `include_directories(${PROJECT_SOURCE_DIR})` but Eigen3 provides its headers via its own target's include directories, not the project source. The global `find_package(Eigen3 REQUIRED)` alone may not propagate to all targets.

**How to avoid:** Add `target_link_libraries(tools PUBLIC Eigen3::Eigen)` and `target_link_libraries(auto_aim PRIVATE Eigen3::Eigen)`.

**Warning signs:** Linker errors for Eigen3 types, or "Eigen/Dense: No such file or directory" when compiling math_tools after `tools` is consumed by `task` or `io`.

### Pitfall 2: Missing Explicit Includes Due to Transitive Inclusion
**What goes wrong:** Source code relies on headers being included transitively (e.g., `<algorithm>` from `<opencv2/opencv.hpp>`, `std::tuple` from OpenCV internal headers). After migration, different OpenCV version or different include order breaks the implicit assumption.

**Why it happens:** The Robocore style guide Section 6.2 explicitly states: "不要依赖 include 的偶然传递。用到什么就显式 include 什么。" (Don't rely on accidental transmission of includes.)

**How to avoid:** Audit all types used in each file and add explicit includes:
- `armor.hpp`: Add `#include <tuple>` (for `std::tuple` in `armor_properties`), `#include <cstddef>` (for `std::size_t`)
- `math_tools.cpp`: Already has `<Eigen/Geometry>`, `<chrono>`, `<cmath>`, `<opencv2/core.hpp>` -- these are fine
- `img_tools.hpp`: Already has `<opencv2/opencv.hpp>`, `<string>`, `<vector>` -- these are fine

**Warning signs:** Compiler errors about unknown types in Phase 3 or later when these headers are included from a different context.

### Pitfall 3: Include Guard Collision
**What goes wrong:** Two files end up with the same include guard, causing one to be silently excluded by the preprocessor.

**Why it happens:** The new guards follow the pattern `TGU_ROBOCORE_2027_<PATH>_<FILE>_HPP`. If two files map to the same guard name (e.g., two files named `config.hpp` in different directories), the first included wins.

**How to avoid:** Use the full module path in the guard name:
- `TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP`
- `TGU_ROBOCORE_2027_TOOLS_MATH_TOOLS_HPP`
- `TGU_ROBOCORE_2027_TOOLS_IMG_TOOLS_HPP`

Also use `#pragma once` as a secondary guard (project style allows both).

## Code Examples

### CMake: tools/CMakeLists.txt (modified)

```cmake
cmake_minimum_required(VERSION 3.16)

add_library(tools STATIC
    crc.cpp
    logger.cpp
    foxglove_comm.cpp
    math_tools.cpp       # NEW
    img_tools.cpp         # NEW
)

target_include_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/tools/foxglove/include
)

target_link_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}/tools/foxglove/lib
)

target_link_libraries(tools PUBLIC
    Eigen3::Eigen         # NEW -- header-only, needed for math_tools
    foxglove
    Boost::boost
    pthread
)
```

`Eigen3::Eigen` is set PUBLIC because downstream targets (like `io`, `app/auto_aim`) that include `tools/math_tools.hpp` will transitively need Eigen3 headers.

### CMake: app/CMakeLists.txt (modified)

```cmake
cmake_minimum_required(VERSION 3.16)

add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
    auto_aim/armor.cpp    # NEW
)

target_include_directories(auto_aim PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(auto_aim PUBLIC
    io
    tools
    Eigen3::Eigen         # NEW -- header-only, needed for armor.hpp
)
```

### Header: app/auto_aim/armor.hpp (adapted)

```cpp
#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP
#pragma once

#include <cstddef>
#include <string>
#include <tuple>
#include <vector>

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

namespace app::auto_aim {

enum Color {
    red,
    blue,
    extinguish,
    purple
};
// ... remainder identical to source, wrapped in app::auto_aim

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP
```

### Source: app/auto_aim/armor.cpp (adapted)

```cpp
#include "app/auto_aim/armor.hpp"

#include <algorithm>
#include <cmath>

#include <opencv2/opencv.hpp>

static constexpr const char* MODULE = "AUTO_AIM";  // per D-09

namespace app::auto_aim {

// Implementations identical to source, just namespace app::auto_aim scope.
// No spdlog calls found in source -- D-06 through D-08 are no-op here.

}  // namespace app::auto_aim
```

### Source: tools/math_tools.cpp (adapted)

```cpp
#include "tools/math_tools.hpp"

#include <cmath>

#include <opencv2/core.hpp>  // CV_PI

static constexpr const char* MODULE = "MATH_TOOLS";

namespace tools {

double limit_rad(double angle) {
    while (angle > CV_PI) angle -= 2 * CV_PI;
    while (angle <= -CV_PI) angle += 2 * CV_PI;
    return angle;
}
// ... all implementations identical to source

}  // namespace tools
```

### Source: tools/img_tools.cpp (adapted)

```cpp
#include "tools/img_tools.hpp"

static constexpr const char* MODULE = "IMG_TOOLS";

namespace tools {

void draw_point(cv::Mat& img, const cv::Point& point, const cv::Scalar& color, int radius) {
    cv::circle(img, point, radius, color, -1);
}
// ... all implementations identical to source

}  // namespace tools
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `namespace auto_aim` | `namespace app::auto_aim` | This phase | All internal references to `Lightbar`, `Armor`, `Color` etc. must use or be inside `app::auto_aim` scope |
| `AUTO_AIM__ARMOR_HPP` | `TGU_ROBOCORE_2027_APP_AUTO_AIM_ARMOR_HPP` | This phase | Include guard collision risk eliminated for cross-project headers |
| `tools::logger()->info(...)` | `LOG_INFO(MODULE, ...)` | This phase (future) | Phase 2 files have no log calls, but pattern established for later phases |

**Deprecated/outdated:**
- **spdlog/fmt**: Not used in Phase 2 files. D-10 confirms no spdlog dependency. The `LOG_XXX` macros use `std::format` internally from `<format>` (C++20), which is available since the project builds as C++20.
- **Relative includes**: Changed to full project-relative paths per D-14. Source files `#include "armor.hpp"` become `#include "app/auto_aim/armor.hpp"`.

## Runtime State Inventory

> **Not applicable.** Phase 2 adds new files to the project. No renaming, refactoring, or migration of existing runtime state. All three modules (armor, math_tools, img_tools) are new additions from sp_vision_25. There are no existing stored data, live service configs, OS registrations, secrets, or build artifacts referencing these names in the Robocore system.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `std::tuple` in `armor_properties` is not explicitly included | Common Pitfalls | Low -- `<opencv2/opencv.hpp>` transitively provides it on both OpenCV 4.6.0 and in sp_vision_25. Adding `#include <tuple>` is a defensive no-op if already included. |
| A2 | Phase 2 source files have zero spdlog calls | Code Examples | Low -- verified by reading the three source .cpp files. No LOG_XXX or tools::logger() references found. D-06 through D-09 are forward-looking for later phases. |
| A3 | `std::size_t` is available implicitly | Common Pitfalls | Low -- `Lightbar::id` uses `std::size_t`. Available via `<opencv2/opencv.hpp>` transitive includes. Safe to add `#include <cstddef>` for correctness. |

## Open Questions

1. **Should auto_aim.hpp/.cpp remain as stubs?**
   - What we know: They exist as empty placeholders from Phase 1.
   - What's unclear: They may be expanded in later phases (e.g., as a facade for the auto_aim pipeline), or may be replaced entirely by armor.hpp for the data layer.
   - Recommendation: Keep them as-is. Phase 2 only adds armor.hpp/.cpp. The stubs remain for Phase 7 (integration) where a top-level interface may be needed.

2. **Are there any spdlog calls in armor.cpp that were missed?**
   - What we know: Visual inspection of armor.cpp shows none. The file only has constructors.
   - What's unclear: None -- verified by reading source.
   - Recommendation: No action needed.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Eigen3 | math_tools (Eigen/Geometry), armor (Eigen/Dense) | yes | 3.4.0 | -- |
| OpenCV | armor (cv::Point, cv::Mat), math_tools (CV_PI), img_tools (drawing) | yes | 4.6.0 | -- |
| CMake | Build system | yes | 3.28.3 | -- |
| g++ | C++20 compilation | yes | 13.3.0 | -- |

**Missing dependencies with no fallback:**
- None -- all required dependencies are installed.

## Validation Architecture

> **Skipped.** `workflow.nyquist_validation` is explicitly `false` in `.planning/config.json`.

## Security Domain

> **Secure by default.** Phase 2 involves only data structure definitions and mathematical utility functions. No network communication, no file I/O (beyond OpenCV drawing helpers), no authentication, no user input processing. No ASVS categories apply.

## Sources

### Primary (HIGH confidence)
- [VERIFIED: source file inspection] sp_vision_25 source files at `/home/eldwen/sp_vision_25/tasks/auto_aim/armor.hpp`, `armor.cpp`, `/home/eldwen/sp_vision_25/tools/math_tools.hpp`, `math_tools.cpp`, `img_tools.hpp`, `img_tools.cpp`
- [VERIFIED: dpkg -l] Eigen3 3.4.0 installed as `libeigen3-dev`
- [VERIFIED: dpkg -l] OpenCV 4.6.0 installed as `libopencv-dev`
- [VERIFIED: source file inspection] Robocore top-level CMakeLists.txt at `/home/eldwen/TGU_Robocore_2027/CMakeLists.txt` (already has `find_package(Eigen3 REQUIRED)`, `find_package(OpenCV REQUIRED)`)
- [VERIFIED: source file inspection] Robocore `项目结构与代码风格.md` at `/home/eldwen/TGU_Robocore_2027/项目结构与代码风格.md`
- [VERIFIED: source file inspection] `tools/logger.hpp` at `/home/eldwen/TGU_Robocore_2027/tools/logger.hpp`

### Secondary (MEDIUM confidence)
- Eigen3 CMake documentation: Eigen3 is header-only; `find_package(Eigen3)` + `target_link_libraries(target Eigen3::Eigen)` is the standard integration pattern. [CITED: Eigen3 documentation]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all versions verified via dpkg and source inspection
- Architecture: HIGH -- all three modules are direct copies with only namespace/include guard changes
- Pitfalls: HIGH -- based on direct analysis of the include graph and build system

**Research date:** 2026-05-01
**Valid until:** 2026-06-01 (stable system dependencies, unlikely to change within 30 days)
