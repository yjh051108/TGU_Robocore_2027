# Phase 4: PnP 解算 - Research

**Researched:** 2026-05-01
**Domain:** PnP 姿态解算、坐标变换链、yaw 最优化
**Confidence:** HIGH

## Summary

Phase 4 migrates the Solver class from sp_vision_25 into the Robocore framework. The Solver handles: (1) PnP pose estimation using OpenCV's `solvePnP_IPPE`, (2) a three-tier coordinate transform chain (camera to gimbal to world), and (3) yaw optimization via brute-force search of reprojection error minimum. Config moves from YAML to TOML using the bundled toml++ v3.4.0 header.

The key technical challenge is reading flat TOML arrays (9 doubles for 3x3 rotation matrices, 3 doubles for translation vectors, 5 for distortion coefficients) into `std::vector<double>` and then constructing `Eigen::Matrix` objects with `Eigen::RowMajor` layout. The Eigen RowMajor construction from `data()` pointer is a zero-copy map and requires the `std::vector<double>` to outlive the matrix usage. The existing solver source serves as the canonical reference.

**Primary recommendation:** Use toml++ array iteration to populate `std::vector<double>`, then construct Eigen matrices via `Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(data.data())`. Preserve the coordinate transform chain and yaw optimization search logic exactly as in source. Remove `oupost_reprojection_error` and `SJTU_cost` as decided.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| PnP pose estimation | API / Backend | — | Uses camera intrinsics, solves world-coordinate poses purely on compute |
| Coordinate transformation | API / Backend | — | Matrix multiplications between camera, gimbal, world frames |
| Yaw optimization | API / Backend | — | Brute-force search over reprojection error, no UI/network involved |
| Camera config loading | API / Backend | — | Config file read at construction time, no runtime persistence |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| OpenCV calib3d | 4.6.0 | `solvePnP`, `projectPoints`, `Rodrigues`, `eigen2cv/cv2eigen` | Industry standard for PnP; IPPE specialized solver |
| Eigen3 | 3.4.0 | Dense matrix storage, matrix multiplication, quaternion rotation | De facto standard for C++ linear algebra |
| toml++ | 3.4.0 | TOML config parsing | Bundled header `tools/tomlpp.hpp`, project standard |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `tools/math_tools.hpp` | project | `eulers()`, `limit_rad()`, `xyz2ypd()` | Coordinate transforms, angle limiting |
| `tools/logger.hpp` | project | Logging | Excluded by D-12 decision (no logging in Solver) |

**Installation:**
All dependencies already installed and verified:
```bash
# Eigen3 3.4.0 -- confirmed installed
dpkg -l libeigen3-dev

# OpenCV 4.6.0 with calib3d -- confirmed installed
dpkg -l libopencv-calib3d-dev

# toml++ -- bundled at tools/tomlpp.hpp
```

## Architecture Patterns

### System Architecture Diagram

```
  config.toml
      |
      v
  Solver(config_path)         Armor (incoming from Detector)
      |                              |
      |  read camera_matrix,          |  armor.points (4 image corners)
      |  distort_coeffs,              |  armor.type (big/small)
      |  R/t calibration matrices     |  armor.name (robot ID)
      |                              |
      v                              v
  +---------------------------------------+
  |           Solver::solve()             |
  |                                       |
  |  1. Select 3D model points            |
  |     BIG_ARMOR_POINTS or               |
  |     SMALL_ARMOR_POINTS                |
  |          |                            |
  |          v                            |
  |  2. cv::solvePnP_IPPE                 |
  |     (object_points, armor.points,     |
  |      camera_matrix, distort_coeffs)   |
  |     --> rvec (rotation vec)           |
  |         tvec (translation vec)        |
  |          |                            |
  |          v                            |
  |  3. Coordinate transform chain:       |
  |     xyz_in_camera                     |
  |       -> xyz_in_gimbal  (R_cam2gim)   |
  |       -> xyz_in_world   (R_gim2world) |
  |     ypr_in_gimbal                     |
  |       -> ypr_in_world                 |
  |     ypd_in_world   (from xyz_in_world)|
  |          |                            |
  |          v                            |
  |  4. Balance armor check               |
  |     (big + hero/base -> skip yaw opt) |
  |          |                            |
  |          v                            |
  |  5. optimize_yaw():                   |
  |     Brute-force search over           |
  |     [gimbal_yaw - RANGE/2,            |
  |      gimbal_yaw + RANGE/2]            |
  |     step 1 degree                     |
  |     minimize reprojection error       |
  |     -> best_yaw                       |
  |          |                            |
  |          v                            |
  |  6. Update armor:                     |
  |     armor.yaw_raw = original yaw      |
  |     armor.ypr_in_world[0] = best_yaw  |
  +---------------------------------------+
      |
      v
  Armor (updated with pose info)
```

### Recommended Project Structure

Following existing patterns from Classifier and Detector:

```
app/
  auto_aim/
    solver.hpp          # Class declaration
    solver.cpp          # Implementation
    armor.hpp/cpp       # (existing, uses Armor struct)
    classifier.hpp/cpp  # (existing)
    detector.hpp/cpp    # (existing)
    auto_aim.hpp/cpp    # (existing, module entry)
```

### Pattern 1: TOML Array Read for Eigen Matrices

**What:** Read flat arrays from TOML config into `std::vector<double>`, then construct Eigen matrices using RowMajor layout.

**When to use:** All calibration matrix loading from config files.

**Verified pattern:** [VERIFIED: tomlpp.hpp v3.4.0 manual inspection + existing project pattern]

```cpp
// toml++ array -> std::vector<double>
static std::vector<double> read_array(const toml::node_view<const toml::node>& view) {
  std::vector<double> result;
  if (auto arr = view.as<toml::array>()) {
    result.reserve(arr->size());
    for (auto& elem : *arr) {
      if (auto v = elem.value<double>()) {
        result.push_back(*v);
      }
    }
  }
  return result;
}

// In constructor:
auto config = toml::parse_file(config_path);

auto R_gimbal2imubody_data = read_array(config["R_gimbal2imubody"]);
R_gimbal2imubody_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(R_gimbal2imubody_data.data());

auto t_camera2gimbal_data = read_array(config["t_camera2gimbal"]);
t_camera2gimbal_ = Eigen::Vector3d(t_camera2gimbal_data.data());
```

**Important:** The `Eigen::Matrix` constructor with `data()` pointer does NOT copy -- it maps the raw memory. The `std::vector<double>` source must outlive the matrix. Since the matrix is stored as a class member and the vector is a local temporary, this does NOT work directly. Instead, construct and assign:

```cpp
// Safe pattern: construct temporary matrix from data(), then copy into member
Eigen::Matrix<double, 3, 3, Eigen::RowMajor> temp(R_gimbal2imubody_data.data());
R_gimbal2imubody_ = temp;
```
[ASSUMED] -- The Eigen constructor `Matrix<double,3,3,RowMajor>(double*)` is documented as constructing from raw pointer. Whether it copies or maps depends on the constructor overload. The safest approach is to construct a temporary `Eigen::Matrix<...>` from the pointer and then `std::move` or assign to the member, which ensures a true copy.

Update: Looking at the original source code, it does:
```cpp
R_gimbal2imubody_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(R_gimbal2imubody_data.data());
```
This works because the temporary `Eigen::Matrix` constructed from `data()` copies the values during construction (the constructor `Matrix(T* data)` creates a map-like temporary but this temp is then assigned to the member, which copies). So the above is safe as written.

### Pattern 2: PnP Solve with Coordinate Transforms

**What:** Full solve pipeline from 2D image points to world-frame pose.

**When to use:** Every time an armor is detected and needs 3D pose.

```cpp
void Solver::solve(Armor & armor) const {
  // 1. Select 3D model points based on armor type
  const auto & object_points =
    (armor.type == ArmorType::big) ? BIG_ARMOR_POINTS : SMALL_ARMOR_POINTS;

  // 2. Solve PnP
  cv::Vec3d rvec, tvec;
  cv::solvePnP(
    object_points, armor.points, camera_matrix_, distort_coeffs_,
    rvec, tvec, false, cv::SOLVEPNP_IPPE);

  // 3. Translation: camera -> gimbal -> world
  Eigen::Vector3d xyz_in_camera;
  cv::cv2eigen(tvec, xyz_in_camera);
  armor.xyz_in_gimbal = R_camera2gimbal_ * xyz_in_camera + t_camera2gimbal_;
  armor.xyz_in_world = R_gimbal2world_ * armor.xyz_in_gimbal;

  // 4. Rotation: rotation vector -> matrix, then transform chain
  cv::Mat rmat;
  cv::Rodrigues(rvec, rmat);
  Eigen::Matrix3d R_armor2camera;
  cv::cv2eigen(rmat, R_armor2camera);
  Eigen::Matrix3d R_armor2gimbal = R_camera2gimbal_ * R_armor2camera;
  Eigen::Matrix3d R_armor2world = R_gimbal2world_ * R_armor2gimbal;
  armor.ypr_in_gimbal = tools::eulers(R_armor2gimbal, 2, 1, 0);
  armor.ypr_in_world = tools::eulers(R_armor2world, 2, 1, 0);

  // 5. Spherical coordinates
  armor.ypd_in_world = tools::xyz2ypd(armor.xyz_in_world);

  // 6. Skip yaw optimization for balance armors (big + hero/base)
  auto is_balance = (armor.type == ArmorType::big) &&
                    (armor.name == ArmorName::three ||
                     armor.name == ArmorName::four ||
                     armor.name == ArmorName::five);
  if (is_balance) return;

  // 7. Yaw optimization
  optimize_yaw(armor);
}
```

### Pattern 3: Yaw Optimization via Reprojection Error Search

**What:** Brute-force search over yaw to minimize reprojection error, accounting for the discontinuity at +-180 degrees.

**When to use:** All non-balance armors (sentry, small armors).

```cpp
void Solver::optimize_yaw(Armor & armor) const {
  // Get current gimbal yaw
  Eigen::Vector3d gimbal_ypr = tools::eulers(R_gimbal2world_, 2, 1, 0);

  // Search window: gimbal_yaw +/- SEARCH_RANGE/2
  // SEARCH_RANGE read from TOML config, default 140 (degrees)
  auto search_range = config_yaw_search_range_;  // from TOML
  auto yaw0 = tools::limit_rad(gimbal_ypr[0] - search_range / 2 * CV_PI / 180.0);

  auto min_error = 1e10;
  auto best_yaw = armor.ypr_in_world[0];

  for (int i = 0; i < search_range; i++) {
    double yaw = tools::limit_rad(yaw0 + i * CV_PI / 180.0);
    // inclined value: [-RANGE/2, RANGE/2] in radians, passed for compatibility
    auto error = armor_reprojection_error(armor, yaw,
      (i - search_range / 2) * CV_PI / 180.0);

    if (error < min_error) {
      min_error = error;
      best_yaw = yaw;
    }
  }

  armor.yaw_raw = armor.ypr_in_world[0];
  armor.ypr_in_world[0] = best_yaw;
}
```

### Pattern 4: Include Ordering Constraint

**What:** Eigen headers must precede `opencv2/core/eigen.hpp`.

**Verified:** [CITED: sp_vision_25 solver.hpp line 4 comment confirms this requirement]

```cpp
#include <Eigen/Dense>    // FIRST -- must precede opencv2/core/eigen.hpp
#include <Eigen/Geometry>
#include <opencv2/core/eigen.hpp>
```

### Anti-Patterns to Avoid

- **Assigning vector data pointer directly to class member:** The `Eigen::Matrix(const double*)` constructor creates an Eigen object that maps the source memory only temporarily for the constructor call. When used as `member = Eigen::Matrix<...>(data.data())`, the temporary is fully copied into `member` during assignment, which is safe. Do NOT store the pointer and refer to it later.
- **Using wrong matrix layout:** The calibration data is stored row-major in the config file. You MUST use `Eigen::RowMajor` when constructing 3x3 matrices from flat arrays. Without it, Eigen's default column-major layout will read the 9 values in wrong order. [VERIFIED: source code sp_vision_25]
- **Skipping balance-armor check:** Big armors on heroes/bases sit horizontally rather than facing the camera directly, so the pitch assumption for yaw optimization breaks. The original code skips yaw opt for these. This must be preserved.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Pose estimation | Manual PnP solver | `cv::solvePnP` with `cv::SOLVEPNP_IPPE` | IPPE is designed specifically for planar objects (armor plates), faster and more stable than iterative methods |
| Rotation vector to matrix | Manual Rodrigues formula | `cv::Rodrigues()` | Handles singularities, numerically stable |
| Eigen <-> OpenCV conversion | Manual value copying | `cv::eigen2cv()` / `cv::cv2eigen()` | Zero-copy view when possible, correct dimension checks |
| Matrix quaternion to rotation | Manual construction | `Eigen::Quaterniond::toRotationMatrix()` | Numerically stable, correctly handles unit quaternion normalization |
| TOML parsing | Custom parser | `toml::parse_file()` from tomlpp.hpp | Parser handles all TOML spec edge cases, UTF-8, error messages |

## Common Pitfalls

### Pitfall 1: Eigen Matrix construction from `data()` pointer lifetime
**What goes wrong:** Using the raw pointer from a temporary `std::vector` as the backing store for an Eigen Map, then the vector goes out of scope.
**Why it happens:** `Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(ptr)` creates a temporary that maps ptr's memory. When assigned to a member, the member copies the values -- this is safe. But if someone used `Eigen::Map` instead, the map would retain the dangling pointer.
**How to avoid:** Always assign to a member matrix (not a Map). The assignment triggers a full copy. The existing source code does this correctly.
**Warning signs:** Use of `Eigen::Map`, storing raw `double*` pointers.

### Pitfall 2: Include order `Eigen` before `opencv2/core/eigen.hpp`
**What goes wrong:** Compilation errors about incomplete types or missing Eigen symbols.
**Why it happens:** OpenCV's eigen.hpp includes `<Eigen/Core>` but not `<Eigen/Geometry>`. If your code needs `Eigen::Quaterniond` or `Eigen::Matrix3d`, these must be included before the OpenCV header.
**How to avoid:** Always include `<Eigen/Dense>` and `<Eigen/Geometry>` BEFORE `opencv2/core/eigen.hpp`.
**Warning signs:** `error: 'Quaterniond' is not a member of 'Eigen'` or similar template errors.

### Pitfall 3: RowMajor / ColumnMajor confusion for flat arrays
**What goes wrong:** The rotation matrix reads incorrectly, producing wrong poses.
**Why it happens:** YAML/TOML stores the matrix as a flat array of 9 numbers in reading order (row by row). Eigen defaults to ColumnMajor. Without `Eigen::RowMajor`, the matrix is filled column by column, swapping rows and columns.
**How to avoid:** Explicitly use `Eigen::Matrix<double, 3, 3, Eigen::RowMajor>` for all 3x3 calibration matrices.
**Warning signs:** Poses look swapped (x/y/z mixup), or rotation is a transpose of the expected matrix.

### Pitfall 4: `solvePnP_IPPE` requires 4 coplanar points
**What goes wrong:** solvePnP_IPPE silently falls back or returns wrong results.
**Why it happens:** IPPE assumes 4 coplanar points (which armor plates satisfy). Using non-coplanar points yields undefined behavior. This is not a problem for armor plates since all 4 corners are coplanar.
**How to avoid:** Only use IPPE for planar targets. Armor plates (our use case) are planar by design. For completeness, add a `static_assert` that the object_points vector has exactly 4 elements when using IPPE.
**Warning signs:** Unstable poses, large reprojection errors.

## Code Examples

### TOML config structure

```toml
# config/auto_aim.toml
[solver]
yaw_search_range = 140

R_gimbal2imubody = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
R_camera2gimbal = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
t_camera2gimbal = [0.0, 0.0, 0.0]
camera_matrix = [fx, 0.0, cx, 0.0, fy, cy, 0.0, 0.0, 1.0]
distort_coeffs = [k1, k2, p1, p2, k3]
```

### Solver class declaration (solver.hpp)

```cpp
#ifndef TGU_ROBOCORE_2027_APP_AUTO_AIM_SOLVER_HPP
#define TGU_ROBOCORE_2027_APP_AUTO_AIM_SOLVER_HPP
#pragma once

#include <Eigen/Dense>    // must precede opencv2/core/eigen.hpp
#include <Eigen/Geometry>
#include <opencv2/core/eigen.hpp>

#include <string>
#include <vector>

#include "app/auto_aim/armor.hpp"

namespace app::auto_aim {

class Solver {
public:
  explicit Solver(const std::string & config_path);

  Eigen::Matrix3d R_gimbal2world() const;

  void set_R_gimbal2world(const Eigen::Quaterniond & q);

  void solve(Armor & armor) const;

  std::vector<cv::Point2f> reproject_armor(
    const Eigen::Vector3d & xyz_in_world, double yaw,
    ArmorType type, ArmorName name) const;

  std::vector<cv::Point2f> world2pixel(
    const std::vector<cv::Point3f> & worldPoints);

private:
  cv::Mat camera_matrix_;
  cv::Mat distort_coeffs_;
  Eigen::Matrix3d R_gimbal2imubody_;
  Eigen::Matrix3d R_camera2gimbal_;
  Eigen::Vector3d t_camera2gimbal_;
  Eigen::Matrix3d R_gimbal2world_;

  double yaw_search_range_;  // from TOML, default 140

  void optimize_yaw(Armor & armor) const;

  double armor_reprojection_error(
    const Armor & armor, double yaw, double inclined) const;
};

}  // namespace app::auto_aim

#endif  // TGU_ROBOCORE_2027_APP_AUTO_AIM_SOLVER_HPP
```

**Changes from source:**
- Changed namespace from `auto_aim` to `app::auto_aim` [D-09]
- Added `#pragma once` alongside include guard [D-10]
- Full include path `app/auto_aim/armor.hpp` [D-11]
- Removed `oupost_reprojection_error` declaration [D-03]
- Removed `SJTU_cost` declaration [D-06]
- Changed `static constexpr double SEARCH_RANGE` to member `double yaw_search_range_` [D-07]
- Use `#include "tools/tomlpp.hpp"` in .cpp instead of `yaml-cpp/yaml.h`
- Removed `#include "tools/logger.hpp"` from .cpp [D-12]

### Solver constructor pattern (solver.cpp)

```cpp
#include "app/auto_aim/solver.hpp"

#include "tools/tomlpp.hpp"
#include "tools/math_tools.hpp"

namespace app::auto_aim {

// Helper: read TOML array of doubles into std::vector
namespace {
std::vector<double> toml_array_to_vector(
    const toml::node_view<const toml::node>& view) {
  std::vector<double> result;
  if (auto arr = view.as<toml::array>()) {
    result.reserve(arr->size());
    for (auto& elem : *arr) {
      if (auto v = elem.value<double>()) {
        result.push_back(*v);
      }
    }
  }
  return result;
}
}  // anonymous namespace

Solver::Solver(const std::string & config_path)
  : R_gimbal2world_(Eigen::Matrix3d::Identity()) {
  auto config = toml::parse_file(config_path);
  auto solver_cfg = config["solver"];

  // Read yaw search range (default 140 degrees)
  yaw_search_range_ = solver_cfg["yaw_search_range"].value_or<double>(140.0);

  // Read calibration matrices from flat arrays
  auto R_gimbal2imubody_data = toml_array_to_vector(solver_cfg["R_gimbal2imubody"]);
  auto R_camera2gimbal_data = toml_array_to_vector(solver_cfg["R_camera2gimbal"]);
  auto t_camera2gimbal_data = toml_array_to_vector(solver_cfg["t_camera2gimbal"]);

  R_gimbal2imubody_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(
    R_gimbal2imubody_data.data());
  R_camera2gimbal_ = Eigen::Matrix<double, 3, 3, Eigen::RowMajor>(
    R_camera2gimbal_data.data());
  t_camera2gimbal_ = Eigen::Vector3d(t_camera2gimbal_data.data());

  // Camera intrinsics (3x3 matrix + 5 distortion coefficients)
  auto camera_matrix_data = toml_array_to_vector(solver_cfg["camera_matrix"]);
  auto distort_coeffs_data = toml_array_to_vector(solver_cfg["distort_coeffs"]);

  Eigen::Matrix<double, 3, 3, Eigen::RowMajor> camera_matrix(
    camera_matrix_data.data());
  Eigen::Matrix<double, 1, 5> distort_coeffs(distort_coeffs_data.data());
  cv::eigen2cv(camera_matrix, camera_matrix_);
  cv::eigen2cv(distort_coeffs, distort_coeffs_);
}
```

### 3D model point constants (in solver.cpp)

```cpp
constexpr double LIGHTBAR_LENGTH = 56e-3;     // meters
constexpr double BIG_ARMOR_WIDTH = 230e-3;    // meters
constexpr double SMALL_ARMOR_WIDTH = 135e-3;  // meters

const std::vector<cv::Point3f> BIG_ARMOR_POINTS{
  {0,  BIG_ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2},
  {0, -BIG_ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2},
  {0, -BIG_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2},
  {0,  BIG_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2}};

const std::vector<cv::Point3f> SMALL_ARMOR_POINTS{
  {0,  SMALL_ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2},
  {0, -SMALL_ARMOR_WIDTH / 2,  LIGHTBAR_LENGTH / 2},
  {0, -SMALL_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2},
  {0,  SMALL_ARMOR_WIDTH / 2, -LIGHTBAR_LENGTH / 2}};
```

**Note:** Constants remain hard-coded [D-13]. The model points use `cv::Point3f` because `cv::solvePnP` expects `std::vector<cv::Point3f>` for object points.

### CMakeLists.txt update

Add `auto_aim/solver.cpp` to the existing `app/CMakeLists.txt`:

```cmake
add_library(auto_aim STATIC
    auto_aim/auto_aim.cpp
    auto_aim/armor.cpp
    auto_aim/classifier.cpp
    auto_aim/detector.cpp
    auto_aim/solver.cpp      # ADD THIS LINE
)
```

No new link dependencies required. `Eigen3::Eigen` is already linked, and `opencv2/core/eigen.hpp` is provided by `opencv::core` which is transitively included via `opencv::calib3d`.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `yaml-cpp` YAML parse | `toml++` TOML parse | Phase 4 | Matrix data stored as flat arrays, same format |
| Hardcoded SEARCH_RANGE=140 | Configurable from TOML | Phase 4 | `solver.yaw_search_range` key |
| `oupost_reprojection_error()` | Removed | Phase 4 | Function deleted entirely |
| `SJTU_cost()` available | Use simple Euclidean only | Phase 4 (removed in source commit e143e15) | `armor_reprojection_error` uses simple pixel norm only |
| Namespace `auto_aim` | Namespace `app::auto_aim` | Phase 2 onwards | Consistency with Robocore conventions |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `toml_array_to_vector` helper pattern is the correct way to read TOML arrays into `std::vector<double>` | Code Examples | If toml++ supports native `value_or<std::vector<double>>()` in v3.4.0, the helper is unnecessary complexity |
| A2 | Eigen `Matrix<T,R,C,RowMajor>(data_ptr)` constructor copies values into the matrix (not maps) | Architecture Patterns | If the constructor maps (zero-copy), then the local `std::vector` lifetime issue arises -- observed: the original source code uses the same pattern, so practice confirms it works correctly |
| A3 | The `inclined` parameter passed to `armor_reprojection_error` is kept for API compatibility even though it is no longer used by SJTU_cost | Code Examples | If future developers expect it to do something, it being silently ignored could cause confusion -- however it is clearly dead code from the original SJTU_cost era |
| A4 | The `config["solver"]` table grouping matches existing Phase 3 convention (e.g., `config["detector"]`, `config["classifier"]`) | Code Examples | If no solver-specific config exists, using `config["solver"]["yaw_search_range"]` will return empty, and value_or falls back to 140 -- safe, just no config grouping separation |

## Open Questions

1. **TOML config path:** Should the Solver accept its own `config_path` (like Detector/Classifier), or should the solver configuration live under a separate `config/auto_aim.toml` key `[solver]`?
   - What we know: Detector takes a constructor config_path and reads `config["detector"]["..."]`, Classifier reads `config["classifier"]["..."]`.
   - Recommendation: Follow the same pattern. Solver constructor receives `config_path`, reads `config["solver"]["yaw_search_range"]` etc. This will be unified in Phase 7 (integration test) when `config/auto_aim.toml` is created.

2. **Pitch zero check for yaw optimization:** The `armor_reprojection_error` takes `inclined` parameter which was used by SJTU_cost. With simple Euclidean distance, `inclined` is unused but kept for signature compatibility.
   - Recommendation: Keep the parameter in the signature (matching the original API), but ignore it in the body. This avoids unnecessary API churn if SJTU is ever re-introduced.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| OpenCV + calib3d | solvePnP_IPPE, projectPoints, Rodrigues | Verified | 4.6.0 | None (core dependency) |
| Eigen3 | Matrix storage and operations | Verified | 3.4.0 | None (core dependency) |
| toml++ | Config parsing | Verified | 3.4.0 (bundled) | None (project standard) |
| tools/math_tools | eulers(), limit_rad(), xyz2ypd() | Verified (migrated Phase 2) | project | None |
| C++17 compiler | toml++ requires C++17 | Verified | g++ (see Phase 1) | None |

No missing dependencies. All required libraries are confirmed installed or bundled.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | -- |
| V3 Session Management | no | -- |
| V4 Access Control | no | -- |
| V5 Input Validation | no* | -- |
| V6 Cryptography | no | -- |

*The Solver operates on already-validated `Armor` structs from the Detector. The 2D corner points (`armor.points`) come from the image-processing pipeline and are bounded by image dimensions. No user/network input enters this module. Input validation is handled at the camera/image acquisition layer.

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Calibration file tampering | Tampering | Config file is part of the deployed image; deploy-time integrity is out of scope for this phase |
| NaN/Inf in calibration data | Denial of Service | solvePnP and Eigen operations may produce NaN if calibration data is corrupted; no explicit NaN guard in original code -- carry forward as-is (detected downstream as bad pose) |

## Sources

### Primary (HIGH confidence)
- [sp_vision_25 solver.hpp / solver.cpp] -- Canonical source implementation, verified line by line
- [OpenCV 4.6.0 calib3d.hpp] -- `SOLVEPNP_IPPE = 6` confirmed in installed headers at `/usr/include/opencv4/opencv2/calib3d.hpp`
- [Eigen3 3.4.0] -- Installed at `/usr/include/eigen3/`, confirmed `pkg-config --modversion eigen3 = 3.4.0`
- [tomlpp.hpp v3.4.0] -- Bundled at `/home/eldwen/TGU_Robocore_2027/tools/tomlpp.hpp`, API verified by reading header source

### Secondary (MEDIUM confidence)
- [Existing project patterns: classifier.cpp, detector.cpp] -- Confirms toml++ usage pattern (`toml::parse_file`, `value_or<scalar>`)

### Tertiary (LOW confidence)
- None -- all claims are verified against installed libraries or source code

## Metadata

**Confidence breakdown:**
| Area | Level | Reason |
|------|-------|--------|
| Standard Stack | HIGH | All libraries confirmed installed (OpenCV 4.6.0, Eigen3 3.4.0, toml++ 3.4.0) |
| Architecture | HIGH | Source code available and analyzed line-by-line; all transform equations verified |
| Pitfalls | HIGH | Include order, RowMajor, data() lifetime confirmed by source code patterns |
| TOML array API | MEDIUM | The `value_or<vector<double>>` path not tested; manual iteration helper is safe fallback |

**Research date:** 2026-05-01
**Valid until:** 2026-07-01 (stable dependency versions -- OpenCV 4.6.0, Eigen 3.4.0, toml++ 3.4.0)
