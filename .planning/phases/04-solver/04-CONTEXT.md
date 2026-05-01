# Phase 4: PnP 解算 - Context

**Gathered:** 2026-05-01
**Status:** Ready for planning

<domain>
## Phase Boundary

将 sp_vision_25 的 Solver（PnP 姿态解算、坐标变换链、yaw 优化）迁移至 Robocore 框架。

</domain>

<decisions>
## Implementation Decisions

### 配置格式
- **D-01:** YAML → TOML，使用 `tools/tomlpp.hpp`
- **D-02:** 矩阵/向量数据存为扁平数组，与 YAML 格式一致

### 前哨站特殊逻辑
- **D-03:** 不迁移 `oupost_reprojection_error` 函数
- **D-04:** 理由：前哨站作为自瞄目标是正常的，但该函数是未使用的重载，去繁就简

### 重投影代价函数
- **D-05:** 使用简单欧氏距离（与原项目一致）
- **D-06:** 不迁移 SJTU_cost（原项目 commit e143e15 已放弃，效果无显著优势）

### yaw 优化参数
- **D-07:** SEARCH_RANGE（搜索范围）改为可配置参数，从 TOML 读取
- **D-08:** 配合步长 1° 的搜索逻辑保持不变

### 其他
- **D-09:** 命名空间 `app::auto_aim`
- **D-10:** include guard 项目格式 + `#pragma once`
- **D-11:** 完整 include 路径
- **D-12:** Solver 无日志调用，无需日志适配
- **D-13:** 3D 模型点常量（BIG_ARMOR_POINTS, SMALL_ARMOR_POINTS）保持硬编码
- **D-14:** 保留 `is_balance` 平衡步兵特判（跳过 yaw 优化）
- **D-15:** 保留 `set_R_gimbal2world()` IMU 接口
- **D-16:** 保留 `world2pixel()` 投影函数
- **D-17:** 保留 `yaw_raw` 调试字段

</decisions>

<canonical_refs>
## Canonical References

### 源项目
- `/home/eldwen/sp_vision_25/tasks/auto_aim/solver.hpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/solver.cpp`

### 目标框架
- `app/auto_aim/armor.hpp` — 已迁移
- `tools/math_tools.hpp` — 已迁移（eulers、limit_rad、xyz2ypd 等）

</canonical_refs>

<code_context>
## Existing Code Insights

### 依赖
- `tools::eulers()` — 旋转矩阵转欧拉角（已迁移）
- `tools::limit_rad()` — 角度限制（已迁移）
- `tools::xyz2ypd()` — 坐标转换（已迁移）
- Eigen3, OpenCV — 已安装

### 注意事项
- Solver 用 `cv::solvePnP_IPPE`，需 OpenCV calib3d 模块（已包含）
- 3D 模型点尺寸基于真实物理尺寸（大装甲 230mm，小装甲 135mm，灯条 56mm）
- SEARCH_RANGE 默认 140°，改为 TOML 可配

</code_context>

<specifics>
## Specific Ideas

- yaw 优化 TOML 配置示例：
  ```toml
  [solver]
  yaw_search_range = 140
  ```

</specifics>

<deferred>
## Deferred Ideas

None

</deferred>

---
*Phase: 4-solver*
*Context gathered: 2026-05-01*
