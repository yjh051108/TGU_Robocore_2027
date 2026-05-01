# Phase 5: EKF 目标跟踪 - Context

**Gathered:** 2026-05-02
**Status:** Ready for planning

<domain>
## Phase Boundary

迁移 EKF 目标跟踪模块：ExtendedKalmanFilter（通用 EKF）、Target（目标状态管理）、Tracker（状态机）、Voter（多帧投票）。

</domain>

<decisions>
## Implementation Decisions

### 全向感知依赖
- **D-01:** 删掉依赖 omniperception 的第二个 `track()` 重载
- **D-02:** Tracker 只保留单相机版的 `track()`
- **D-03:** 未来哨兵模式在独立分支上开发

### EKF 位置
- **D-04:** ExtendedKalmanFilter 放 `tools/`（通用算法，不绑定自瞄业务）
- **D-05:** Target/Tracker/Voter 放 `app/auto_aim/`

### Target 特判
- **D-06:** 保留 `is_balance` 平衡步兵的 2 装甲板特判
- **D-07:** 注释掉 `outpost` 前哨站 3 装甲板特判
- **D-08:** 注释掉 `base` 基地 3 装甲板特判
- **D-09:** 其他机器人统一使用 4 装甲板逻辑

### Tracker 配置
- **D-10:** YAML → TOML，使用 `tools/tomlpp.hpp`
- **D-11:** 注释掉 `outpost_max_temp_lost_count` 配置项
- **D-12:** 日志 `tools::logger()` → `LOG_XXX`

### 图像中心排序
- **D-13:** 硬编码 `img_center(1440/2, 1080/2)` 改为使用 `center_norm` 归一化坐标
- **D-14:** 改为 `cv::Point2f(0.5, 0.5)`，与分辨率无关

### Voter
- **D-15:** 完整迁移，无改动

</decisions>

<canonical_refs>
## Canonical References

### 源项目
- `/home/eldwen/sp_vision_25/tools/extended_kalman_filter.hpp`
- `/home/eldwen/sp_vision_25/tools/extended_kalman_filter.cpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/target.hpp/.cpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/tracker.hpp/.cpp`
- `/home/eldwen/sp_vision_25/tasks/auto_aim/voter.hpp/.cpp`

### 目标框架
- `tools/math_tools.hpp` — 已迁移
- `app/auto_aim/armor.hpp` — 已迁移
- `app/auto_aim/solver.hpp` — 已迁移

</canonical_refs>

<code_context>
## Existing Code Insights

### EKF 依赖链
- EKF → Eigen3（纯头文件库，已在顶层 CMake 中）
- Target → EKF, math_tools, armor
- Tracker → Solver, Target, armor
- Voter → armor 枚举

### 注意事项
- Target 的 `tools::logger()->debug()` 需要改为 `LOG_DEBUG`
- Tracker 的 `tools::logger()->warn/debug` 需要改为 `LOG_WARN/LOG_DEBUG`
- Voter 无日志调用

</code_context>

<specifics>
## Specific Ideas

- cmake: tools/ 添加 EKF 源文件，target_link_libraries 不需要额外依赖（Eigen3 已链接）
- Tracker 排序用 center_norm 代替硬编码分辨率

</specifics>

<deferred>
## Deferred Ideas

- 哨兵多相机模式（omniperception）— 未来独立分支开发
- TODO: Tracker 的 omniperception 第二个 `track()` 重载已删除，如需哨兵模式：
  `git checkout -b sentry && git revert <commit-that-removed-overload>`

</deferred>

---
*Phase: 5-tracking*
*Context gathered: 2026-05-02*
