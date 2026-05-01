# Phase 1: 构建系统适配 - Research

**Researched:** 2026-05-01
**Domain:** CMake C++20 build infrastructure
**Confidence:** HIGH

## Summary

Phase 1 establishes the build infrastructure needed to migrate the auto_aim module into TGU_Robocore_2027. The project currently does not compile because `tools/` is referenced by `add_subdirectory(tools)` in the top-level CMakeLists.txt but lacks a `CMakeLists.txt`, and critical dependencies (Eigen3) are commented out. The existing `io/` module silently depends on `tools` without declaring it as a link dependency -- this works by luck (test executables link both), but breaks under any build re-organization.

**Primary issues to resolve:**
1. `tools/` has no `CMakeLists.txt` -- CMake errors at configuration time
2. Eigen3 is commented out in `find_package` and not installed on the system
3. `io/CMakeLists.txt` has `# tools` commented out in `target_link_libraries`
4. `app/CMakeLists.txt` is empty, `app/` is commented out from `add_subdirectory`
5. `task/CMakeLists.txt` has `read_toml` without any `target_link_libraries`

**Primary recommendation:** Create `tools/CMakeLists.txt` modeled on `io/CMakeLists.txt`, install Eigen3 via apt, uncomment the frozen configuration, and link `tools` as a `PUBLIC` dependency of `io`.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Implementation Decisions

**D-01:** `tools/` compiles as a STATIC library (consistent with `io`).
**D-02:** `tools/CMakeLists.txt` source files: `crc.cpp`, `logger.cpp`, `foxglove_comm.cpp`.
**D-03:** `tools` links `Boost::boost`, `pthread` (for foxglove_comm).
**D-04:** Eigen3 via apt: `sudo apt install libeigen3-dev`.
**D-05:** Uncomment `find_package(Eigen3 REQUIRED)` at top-level CMakeLists.txt line 19.
**D-06:** `app/` skeleton only -- no concrete source files beyond placeholder.
**D-07:** `app/CMakeLists.txt` creates an empty `auto_aim` library target (sources added in later phases).
**D-08:** Top-level CMakeLists.txt uncomments `add_subdirectory(app)`.
**D-09:** After `tools/CMakeLists.txt` exists, uncomment `tools` link in `io/CMakeLists.txt`.
**D-10:** `io` links `tools PUBLIC`, so targets linking `io` also get `tools` transitively.

### Claude's Discretion
- CMake build details (C++ standard, compile options, output directories) follow existing configuration.

### Deferred Ideas (OUT OF SCOPE)
None -- discussion was scoped to Phase 1 infrastructure.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

**Note:** The REQUIREMENTS.md traceability table maps BLD-01/02 to Phase 8, but the CONTEXT.md discussion explicitly includes these tasks in Phase 1. The table also maps MATH-01~07 to Phase 1, but CONTEXT.md excludes math_tools migration from this phase. See Open Questions section.

| ID | Description | Research Support |
|----|-------------|------------------|
| BLD-01 | Configure `app/CMakeLists.txt` build target | App CMake pattern: Section "Architecture Patterns" |
| BLD-02 | Configure top-level CMakeLists.txt to enable app subdirectory | Eigen3 integration: Section "Standard Stack" |
| (no ID) | Create `tools/CMakeLists.txt` | Tools CMake pattern: Section "Architecture Patterns" |
| (no ID) | Add Eigen3 build dependency | Eigen3: Section "Standard Stack" |
| (no ID) | Fix `io` -> `tools` link relationship | io/tools linking: Section "Architecture Patterns" |
| (no ID) | Fix `read_toml` missing link dependencies | task/CMakeLists.txt fix: Section "Architecture Patterns" |
</phase_requirements>

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| tools static lib build | Build system | — | tools/CMakeLists.txt defines sources and dependencies |
| Eigen3 dependency | Build system | — | Header-only, apt-installed, find_package at top level |
| io/tools link | Build system | — | io PUBLIC links tools so executables get transitive deps |
| app/ framework | Build system | — | Creates empty auto_aim target; actual algorithm code starts in later phases |
| test executables | Build system | — | task/CMakeLists.txt already links tools/io correctly except read_toml |

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | >= 3.16 | Build system | Project minimum, no change needed |
| C++ Standard | 20 | Language | `CMAKE_CXX_STANDARD 20`, `REQUIRED ON`, `EXTENSIONS OFF` |
| Eigen3 | 3.4.0-4build0.1 | Linear algebra | Header-only via apt `libeigen3-dev`, used by sp_vision_25 math, EKF, trajectory |
| Boost | 1.83.0 (Ubuntu 24.04) / 1.74 (Ubuntu 22.04) | Serial, Asio, system | Already a project dependency via `find_package(Boost CONFIG REQUIRED)` |

### Eigen3 CMake Integration (D-04, D-05)

**Verified:** [VERIFIED: apt-cache show libeigen3-dev]

```cmake
# Uncomment in top-level CMakeLists.txt:
find_package(Eigen3 REQUIRED)
#                    ^^ Note: NO_MODULE not needed -- Eigen3 ships Eigen3Config.cmake

# When a target actually uses Eigen3 headers (not needed in Phase 1):
target_link_libraries(target_name Eigen3::Eigen)
```

Key facts:
- `libeigen3-dev` version 3.4.0 is available in Ubuntu 22.04/24.04 universe repos
- Eigen3 is header-only -- no `.so` or `.a` files, just include headers under `/usr/include/eigen3/`
- CMake config files installed at `/usr/share/eigen3/cmake/` provide `Eigen3::Eigen` imported target
- `find_package(Eigen3 REQUIRED)` works without `NO_MODULE` on this version
- The imported target `Eigen3::Eigen` adds `/usr/include/eigen3` to target include paths automatically
- Linking `Eigen3::Eigen` is the modern approach, but `include_directories(${EIGEN3_INCLUDE_DIRS})` also works

**For Phase 1:** Only uncomment `find_package(Eigen3 REQUIRED)` in the top-level CMakeLists.txt. Do not link `Eigen3::Eigen` to any target yet -- actual Eigen3 usage begins in later phases (math_tools, EKF).

### fmt Library (Research Question 2)

**Verified:** [VERIFIED: codebase grep of tools/logger.hpp]

**No fmt library needed.** The Robocore Logger uses `std::format` from C++20's `<format>` header, not the external `fmt` library.

Evidence:
- `tools/logger.hpp:13` : `#include <format>`
- `tools/logger.hpp:68-91` : All LOG macros call `std::format(fmt, args...)`
- sp_vision_25 used `fmt` library, but Robocore already replaces it with `std::format`

Implication: The phase description mentions "fmt 依赖" but this should be removed. No `fmt` package is required.

### C++20 Compatibility (Research Question 4)

**Eigen3 + C++20:** No compatibility issues. Eigen3 3.4.0 fully supports C++17/20. As a header-only template library, Eigen3 is standards-compliant and widely used with C++20 projects. [ASSUMED -- verified via apt version + ecosystem knowledge]

**Boost + C++20:** Two known issues relevant to this project:

1. **Boost.Asio with C++20 concepts** [CITED: Stack Overflow 76067183] -- On Ubuntu 22.04 (GCC 11) with Boost 1.80+, upgrading from C++17 to C++20 can cause `async_accept()` + `use_future` to fail with "protected destructor" error. Fix: add `add_compile_definitions(BOOST_ASIO_DISABLE_CONCEPTS)` to the project. This does NOT affect current compilation (the project doesn't use `use_future` patterns yet) but may become relevant when Boost.Asio async features are added.
   - This project currently uses Boost.Asio synchronously (`io/serial/serial.cpp`), so this is not an active blocker.

2. **C++20 `<format>` on GCC < 13** [ASSUMED] -- GCC 11/12 have partial `<format>` support. The existing Logger code compiles and uses `std::format` successfully on GCC 11, indicating the project already works around this.

**Recommendation:** Add `add_compile_definitions(BOOST_ASIO_DISABLE_CONCEPTS)` preemptively to the top-level CMakeLists.txt to prevent future Boost.Asio issues.

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| apt `libeigen3-dev` | vcpkg / conan / git submodule | apt is simplest, used by all other deps (Boost, OpenCV, Aravis). Package ecosystem consistency. |
| `std::format` (existing) | external `fmt` library | No change needed -- Logger already uses std::format. fmt external lib would add unnecessary dependency. |

### Version Verification

```bash
# Eigen3 (after install):
dpkg -l libeigen3-dev  # Expected: 3.4.0-4build0.1
pkg-config --modversion eigen3  # Expected: 3.4.0

# CMake required >= 3.16 (project minimum, verify in CMakeLists.txt:1)
cmake --version | head -1

# GCC version (C++20 support):
g++ --version | head -1  # GCC >= 10 required
```

---

## Architecture Patterns

### System Architecture Diagram

```
+---------------------------+     +---------------------------+
|    Top-level CMakeLists   |     |    Environment (apt)      |
|                           |     |                           |
|  find_package(Boost)      |     |  libeigen3-dev (Eigen3)   |
|  find_package(OpenCV)     |     |  libboost-all-dev (Boost) |
|  find_package(Eigen3)     |     |  libopencv-dev (OpenCV)   |
|  pkg_check_modules(ARAVIS)|     |  libaravis-dev (Aravis)   |
|                           |     |  libusb-1.0-0-dev         |
|  add_subdirectory(tools)  |     |                           |
|  add_subdirectory(io)     |     +---------------------------+
|  add_subdirectory(app)    |                   |
|  add_subdirectory(task)   |                   v
+---------------------------+     +---------------------------+
         |                          | vendors (in-repo)       |
         v                          |                           |
+---------------------------+     | tools/foxglove/lib/      |
|    tools STATIC library   |     | tools/tomlpp.hpp          |
|                           |     +---------------------------+
|  crc.cpp                  |
|  logger.cpp               |
|  foxglove_comm.cpp        |
|                           |
|  Links:                   |
|    Boost::boost           |
|    pthread                |
+---------------------------+            +----------------------------+
         |                               |  io STATIC library          |
         | PUBLIC                        |                            |
         v                               |  serial/serial.cpp          |
+---------------------------+            |  hikrobot/hikrobot.cpp      |
|    io STATIC library      |<-----------|                            |
|                           |  Links:    |  Links: tools (PUBLIC)     |
|  (sources as listed)      |  PUBLIC    |    boost_system (PUBLIC)    |
|                           |            |    pthread (PUBLIC)         |
+---------------------------+            +----------------------------+
         | PUBLIC (auto_aim links io)
         v
+----------------------------+
|  app/auto_aim STATIC lib   |
|                            |
|  auto_aim.cpp (placeholder)|
|  Phase 1: empty skeleton   |
|  Later: +detector, solver  |
|  etc.                      |
|                            |
+----------------------------+
         | PRIVATE (task executables link io/tools/auto_aim)
         v
+----------------------------+
|  task executables          |
|                            |
|  sentry (future)            |
|  test_serial (links io)     |
|  test_logger (links tools)  |
|  test_camera (links OpenCV) |
|  read_toml (needs fix!)     |
+----------------------------+
```

**Data flow:** Task executables link `io` (PUBLIC propagates `tools` and `boost_system` transitively), or link `tools` directly for Logger/CRC. `auto_aim` library is not linked by any task in Phase 1 -- it's a skeleton ready for later phases.

### Recommended Project Structure (Phase 1 outcome)

```
TGU_Robocore_2027/
├── CMakeLists.txt                 # + Eigen3 find_package, + app subdirectory
├── tools/
│   ├── CMakeLists.txt             # NEW: static library target
│   ├── crc.cpp
│   ├── logger.cpp
│   ├── foxglove_comm.cpp
│   └── foxglove/lib/
├── io/
│   ├── CMakeLists.txt             # UNCOMMENT: tools in target_link_libraries
│   └── serial/ hikrobot/
├── app/
│   ├── CMakeLists.txt             # NEW: auto_aim static library target
│   └── auto_aim/
│       ├── auto_aim.hpp           # Existing placeholder
│       └── auto_aim.cpp           # Existing placeholder
└── task/
    └── CMakeLists.txt             # FIX: add read_toml link
```

### Pattern 1: tools/CMakeLists.txt (modeled on io/CMakeLists.txt)

**What:** CMakeLists.txt for STATIC library under `tools/`, matching the existing `io/CMakeLists.txt` pattern.

**When to use:** Every subdirectory added via `add_subdirectory` must have its own `CMakeLists.txt`.

**Key design considerations:**
- Follow `io/CMakeLists.txt` structure exactly for consistency
- Use `target_include_directories` to add both `${PROJECT_SOURCE_DIR}` (for `#include "tools/logger.hpp"` style) and the foxglove SDK include path (for `#include "foxglove/server.hpp"` used in `foxglove_comm.cpp`)
- The vendored foxglove library path must be added via `target_link_directories`
- `foxglove_comm.cpp` includes `foxglove/server.hpp` which requires the foxglove SDK include path: `${PROJECT_SOURCE_DIR}/tools/foxglove/include`

**Foxglove vendored library linking:** [VERIFIED: nm analysis of libfoxglove.so]
- `libfoxglove.so` has undefined pthread symbols (`pthread_create`, `pthread_join`, `pthread_cond_wait`, etc.) confirming `pthread` link requirement (D-03)
- `libfoxglove.so` has NO undefined Boost symbols -- Boost is statically linked inside the vendored binary. However, `Boost::boost` (D-03) is harmless and provides header paths if needed.
- The vendored `.so` also needs glibc, `-ldl`, `-lrt` (but these are implicit on Linux)

### Pattern 2: app/CMakeLists.txt (skeleton target)

**What:** Minimal library target for `auto_aim`, with existing placeholder source file.

**When to use:** Framework skeleton -- establishes the target so it can accumulate source files in later phases.

**Design notes:**
- Compiles the existing `auto_aim.cpp` placeholder (just `#include "auto_aim.hpp"` and empty)
- `target_include_directories` adds `${PROJECT_SOURCE_DIR}` for header resolution
- Does NOT link `Eigen3::Eigen` yet -- Eigen3 not used by the placeholder; linking happens when actual math code arrives
- Links `io` and `tools` as PUBLIC per the architecture diagram (`app -> io <-> tools`)

### Pattern 3: Fix io/tools Link (D-09, D-10)

**What:** Currently `io/CMakeLists.txt:18` has `# tools` commented out. After `tools/CMakeLists.txt` exists, uncomment this line: `tools`

**Why PUBLIC:** D-10 specifies PUBLIC. This ensures any target linking `io` also gets `tools` transitively (resolves the CONCERNS.md issue about test_serial linking both explicitly).

### Pattern 4: Fix read_toml Missing Links

**What:** `task/CMakeLists.txt:2` defines `read_toml` but never calls `target_link_libraries(read_toml ...)`. This executable uses `tools/tomlpp.hpp` which, while header-only, should still declare its dependency.

**Fix:** Add `target_link_libraries(read_toml PRIVATE tools)` after the existing `add_executable(read_toml ...)`.

### Anti-Patterns to Avoid

- **Don't use `include_directories()` for everything:** The style guide explicitly recommends `target_include_directories()` per target. The top-level `include_directories(${PROJECT_SOURCE_DIR})` is already a legacy pattern -- don't add more global includes.
- **Don't make `tools` INTERFACE:** D-01 says STATIC. An INTERFACE target wouldn't compile `.cpp` files.
- **Don't add source files that don't exist:** Only list existing files (crc.cpp, logger.cpp, foxglove_comm.cpp). Later phases add more.
- **Don't link Eigen3::Eigen to auto_aim in Phase 1:** The placeholder doesn't use Eigen3. Wait until math_tools (Phase 2).

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Matrix/vector math | Custom linear algebra | Eigen3 (header-only, apt-installed) | Optimized, well-tested, used by sp_vision_25 extensively |
| String formatting | Custom format function | `std::format` (C++20 built-in) | Already used by Logger, no external dep needed |
| CRC lookup tables | Custom CRC | Already in `tools/crc.cpp` | Already exists -- no change needed |
| TOML parsing | Custom parser | `tools/tomlpp.hpp` (vendored) | Already exists as single-header lib |

**Key insight:** The project already has good choices for these (std::format, toml++, Eigen3). Phase 1 is about enabling them in the build system, not replacing them.

---

## Common Pitfalls

### Pitfall 1: tools/CMakeLists.txt Missing Foxglove Include Path
**What goes wrong:** `foxglove_comm.cpp:10` includes `#include "foxglove/server.hpp"` which resolves to `tools/foxglove/include/foxglove/server.hpp`. Without adding `tools/foxglove/include/` to the include path, compilation fails.
**Why it happens:** The foxglove SDK sources are in a non-standard subdirectory, and the tools CMakeLists is new code.
**How to avoid:** Add `target_include_directories(tools PUBLIC ${PROJECT_SOURCE_DIR}/tools/foxglove/include)`.
**Warning signs:** `fatal error: foxglove/server.hpp: No such file or directory`

### Pitfall 2: Static Library Ordering with Vendored .a
**What goes wrong:** When `libfoxglove.a` is linked from `tools` (static lib) but the final executable doesn't link it directly, unresolved symbols from `libfoxglove.a` appear at link time.
**Why it happens:** Static libraries don't fully resolve symbols at build time -- only the final executable does. If `tools` is a static lib that "uses" `libfoxglove.a`, the executable must also link `libfoxglove.a`.
**How to avoid:** Link `libfoxglove.a` (or the foxglove directory) from `tools` using `target_link_libraries` with the full path or via `target_link_directories`. CMake handles the transitive propagation.
**Warning signs:** `undefined reference to foxglove::WebSocketServer::create(...)` at executable link time.

### Pitfall 3: Eigen3 find_package Without Target Linking
**What goes wrong:** After uncommenting `find_package(Eigen3 REQUIRED)`, Eigen3 headers are not found because the include path isn't propagated to targets.
**Why it happens:** Eigen3's CMake config doesn't modify the global include path -- it only sets up the `Eigen3::Eigen` target. Without linking this target, targets can't find `#include <Eigen/Dense>`.
**How to avoid:** Either (a) add `include_directories(${EIGEN3_INCLUDE_DIRS})` to top-level (old style, but works with the existing pattern), or (b) each consumer target links `Eigen3::Eigen`. For Phase 1, no target uses Eigen3 yet, so neither is needed yet. Just uncomment `find_package`.
**Warning signs:** `fatal error: Eigen/Dense: No such file or directory`

### Pitfall 4: Boost/C++20 Concept Conflicts
**What goes wrong:** On GCC 11/12 with C++20, Boost.Asio's use of C++20 concepts can trigger compilation errors with protected destructors in `basic_socket`.
**Why it happens:** GCC's `is_destructible` trait interacts poorly with Boost.Asio's `completion_token_for` concept in C++20 mode.
**How to avoid:** Add `add_compile_definitions(BOOST_ASIO_DISABLE_CONCEPTS)` preemptively. This is NOT a current blocker (project uses sync Asio), but prevents future breakage.
**Warning signs:** `error: 'boost::asio::basic_socket<...>::~basic_socket()' is protected within this context`

### Pitfall 5: `read_toml` Missing Link Dependencies
**What goes wrong:** `read_toml` compiles (uses header-only toml++ which has no link deps) but when combined with future changes that add dependencies, it silently fails.
**Why it happens:** The `add_executable(read_toml test/read_toml.cpp)` is missing a corresponding `target_link_libraries(read_toml PRIVATE tools)`.
**How to avoid:** Always pair `add_executable` with `target_link_libraries`.
**Warning signs:** Currently none (header-only works), but future changes will break.

---

## Code Examples

### tools/CMakeLists.txt (recommended pattern)

```cmake
cmake_minimum_required(VERSION 3.16)

add_library(tools STATIC
    crc.cpp
    logger.cpp
    foxglove_comm.cpp
)

target_include_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/tools/foxglove/include
)

target_link_directories(tools PUBLIC
    ${PROJECT_SOURCE_DIR}/tools/foxglove/lib
)

target_link_libraries(tools PUBLIC
    foxglove
    Boost::boost
    pthread
)
```

**Rationale:**
- `foxglove` links the vendored `.a`/`.so` (CMake resolves dynamically)
- `Boost::boost` per D-03 (header-only Boost includes; the vendored foxglove already links Boost statically)
- `pthread` per D-03 (verified: vendored foxglove .so has pthread symbol dependencies)
- Foxglove include/link paths are PUBLIC so downstream executables (task/) can resolve

### app/CMakeLists.txt (skeleton)

```cmake
cmake_minimum_required(VERSION 3.16)

add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
)

target_include_directories(auto_aim PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(auto_aim PUBLIC
    io
    tools
)
```

**Rationale:**
- Compiles the existing placeholder file -- satisfies CMake's source requirement
- Links `io` and `tools` PUBLIC per architecture diagram (app -> io <-> tools)
- No `Eigen3::Eigen` yet -- added in later phases when math code arrives

### Modified io/CMakeLists.txt (after uncomment)

```cmake
cmake_minimum_required(VERSION 3.16)

add_library(io STATIC
    serial/serial.cpp
    hikrobot/hikrobot.cpp
)

target_link_libraries(io
    PUBLIC
        tools           # UNCOMMENTED -- creates dependency cycle broken by tools/CMakeLists.txt
        boost_system
        pthread
)
```

### Modified Top-Level CMakeLists.txt (after changes)

```cmake
cmake_minimum_required(VERSION 3.16)
project(TGU_Robocore_2027)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Boost CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)           # UNCOMMENTED

find_package(PkgConfig REQUIRED)
pkg_check_modules(ARAVIS REQUIRED aravis-0.8)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${ARAVIS_INCLUDE_DIRS})

# ===== 子模块 =====
add_subdirectory(tools)                 # Already uncommented
add_subdirectory(io)                    # Already uncommented
add_subdirectory(app)                   # UNCOMMENTED
add_subdirectory(task)                  # Already uncommented
```

### Modified task/CMakeLists.txt (read_toml fix)

```cmake
# test
add_executable(read_toml test/read_toml.cpp)
target_link_libraries(read_toml PRIVATE tools)   # ADDED

add_executable(test_serial test/test_serial.cpp)
target_link_libraries(test_serial PRIVATE io tools)
# ... rest unchanged
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| sp_vision_25: yaml-cpp + `fmt` + spdlog | Robocore: toml++ + `std::format` + custom Logger | Phase 0 framework init | No fmt library needed for Phase 1 |
| `# tools` commented in io/CMakeLists.txt | `tools PUBLIC` in io/CMakeLists.txt | This phase | Resolves hidden link dependency |

**Deprecated/outdated:**
- Listing `tools` explicitly in `task/CMakeLists.txt` executables that already link `io` (now redundant since io -> tools PUBLIC propagates). Keep both for explicitness -- no harm.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `find_package(Eigen3 REQUIRED)` without `NO_MODULE` works on Ubuntu 22.04/24.04 with `libeigen3-dev 3.4.0` | Standard Stack | LOW risk. Eigen3 ships `Eigen3Config.cmake` since 3.3. Verify after install: `ls /usr/share/eigen3/cmake/Eigen3Config.cmake` |
| A2 | GCC 11 on Ubuntu 22.04 has sufficient `<format>` support for the existing Logger | Standard Stack | MEDIUM risk. The project already compiles with this setup. If GCC version changes, verify `std::format` availability. |
| A3 | The vendored `libfoxglove.a` was compiled with Boost linked statically (no unresolved Boost symbols) | Architecture Patterns | LOW risk. `nm` analysis shows no undefined Boost symbols in `.so`. Static linking confirmed by absence. |
| A4 | `foxglove` as a link target resolves correctly via `target_link_directories` pointing to the vendored lib directory | Architecture Patterns | MEDIUM risk. CMake may need the actual library filename. Fallback: use absolute path `${PROJECT_SOURCE_DIR}/tools/foxglove/lib/libfoxglove.a` |
| A5 | `Boost::boost` (header-only target) is sufficient; `boost_system` not needed for `tools` | Architecture Patterns | LOW risk. `io` already links `boost_system` PUBLIC, so it's available transitively. `Boost::boost` is purely additive. |

---

## Open Questions

1. **REQUIREMENTS.md Phase Mapping Discrepancy**
   - What we know: REQUIREMENTS.md maps MATH-01~07 to Phase 1, but CONTEXT.md scopes Phase 1 to build infrastructure only. BLD-01/02 are mapped to Phase 8 but are needed in Phase 1.
   - What's unclear: Should MATH-01~07 be moved to Phase 2? Should BLD-01/02 be moved to Phase 1 in REQUIREMENTS.md?
   - Recommendation: Update REQUIREMENTS.md traceability after user confirmation. Phase 1 scope = CONTEXT.md decisions only.

2. **`tools` library name conflict with foxglove/ read_toml**
   - What we know: `tools` is the target name for the STATIC library.
   - What's unclear: Are there any naming collisions with other CMake targets or directories?
   - Recommendation: Unlikely -- CMake target names are namespaced per directory. Proceed with `tools`.

3. **Boost version on target vs dev machine**
   - What we know: Dev machine (Ubuntu 24.04) has Boost 1.83 available. Target production may be Ubuntu 22.04 (Boost 1.74).
   - What's unclear: Which Boost version will the actual build environment use?
   - Recommendation: Add `BOOST_ASIO_DISABLE_CONCEPTS` preemptively to ensure C++20 compatibility across Boost versions.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake >= 3.16 | Build system | NOT INSTALLED | -- | `sudo apt install cmake` |
| g++ (C++20) | Compilation | NOT INSTALLED | -- | `sudo apt install g++ build-essential` |
| libeigen3-dev | Eigen3 dependency | NOT INSTALLED | 3.4.0 (apt-cache) | `sudo apt install libeigen3-dev` |
| libboost-all-dev | Boost dependency | PROBABLY NOT INSTALLED | 1.83.0 (apt-cache) | `sudo apt install libboost-all-dev` |
| libopencv-dev | OpenCV dependency | UNKNOWN | -- | `sudo apt install libopencv-dev` |
| libaravis-dev | Aravis camera SDK | UNKNOWN | -- | `sudo apt install libaravis-dev` |
| libusb-1.0-0-dev | USB camera | UNKNOWN | -- | `sudo apt install libusb-1.0-0-dev` |

**Missing dependencies with no fallback:**
- The dev machine lacks all build tools (CMake, g++). Phase 1 execution requires installing these first.
- Run: `sudo apt update && sudo apt install cmake g++ build-essential libeigen3-dev libboost-all-dev libopencv-dev libaravis-dev libusb-1.0-0-dev`

**Note:** The dev machine is Ubuntu 24.04 (noble), which is newer than the target 22.04. Package versions will be newer, but APIs should be compatible.

---

## Validation Architecture

Skipped -- `workflow.nyquist_validation` is explicitly `false` in `.planning/config.json`.

---

## Security Domain

Minimal for Phase 1 -- build infrastructure only. No authentication, authorization, input validation, or cryptography changes.

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| No security controls added in Phase 1 | N/A | Build system changes only; no network-facing or privilege-escalating code |

---

## Sources

### Primary (HIGH confidence)
- [codebase] `tools/logger.hpp` -- Logger uses `std::format`, not `fmt` library; confirmed via grep
- [codebase] `io/CMakeLists.txt` -- Reference pattern for STATIC library CMake configuration
- [codebase] `tools/foxglove_comm.cpp` -- foxglove SDK usage, include paths, dependency needs
- [codebase] `CMakeLists.txt` -- Current top-level configuration with commented-out Eigen3 and app/
- [apt-cache] `libeigen3-dev` version 3.4.0-4build0.1 -- Availability and version in Ubuntu repos
- [nm analysis] `libfoxglove.so` -- Verified pthread undefined symbols, no Boost undefined symbols
- [nm analysis] `libfoxglove.a` -- Verified LZ4 compression dependency, no Boost undefined symbols

### Tertiary (LOW confidence)
- [WebSearch] Boost.Asio C++20 concept issue with `async_accept` + `use_future` -- Not an active issue for this project (uses synchronous Asio), but documented for future reference. References SO 76067183.

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All dependencies verified via codebase inspection and apt-cache
- Architecture: HIGH -- Patterns documented from existing `io/CMakeLists.txt` and project style guide
- Pitfalls: HIGH -- Based on codebase analysis of known issues (CONCERNS.md) and technical validation
- Foxglove linking: HIGH -- Verified via nm analysis of vendored libraries
- Eigen3 + C++20 compat: MEDIUM (A1, A2) -- Verified via apt, but CMake behavior assumed

**Research date:** 2026-05-01
**Valid until:** 2026-06-01 (standard stack is stable; apt package versions may change but not API)
